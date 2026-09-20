/*
 * Device-owned libsdmextension.so for gts7xllite (SM-T736B).
 *
 * The stock libsdmextension.so blob was built for Android 11 and shares
 * internal SDM structure layouts (Layer, LayerBufferMap, HWLayersInfo)
 * with the Android-11 libsdmcore blob. The stock strategy pushes Layer
 * objects compiled against that older layout into HWLayersInfo::hw_layers;
 * the source-built libsdmcore (hardware/qcom-caf/sm8250/display) reads them
 * with the Android-14 layout (which adds Layer::buffer_map) and crashes in
 * HWDeviceDRM::Registry::MapBufferToFbId(). The blob therefore cannot be
 * loaded behind the source-built core at all.
 *
 * The built-in fallback path (extension_intf_ == nullptr) is equally not
 * usable: nothing in the open-source core clears HWLayersInfo::hw_layers
 * per frame - the proprietary StrategyImpl::Start() normally does that -
 * so hw_layers would accumulate forever and ResourceDefault::Prepare()
 * rejects every frame after the first.
 *
 * This library therefore implements the extension contract in source,
 * compiled against the same headers as libsdmcore:
 *
 *   - CreateResourceExtn() delegates to ResourceDefault, the generic
 *     resource manager already compiled into libsdmcore.
 *   - CreateStrategyExtn() supplies a minimal GPU-composition strategy:
 *     it clears the per-frame vectors (the missing piece above) and marks
 *     all application layers for GPU composition, equivalent to the
 *     core's built-in GetNextStrategy() fallback.
 *   - Partial update / DPPS extensions are not provided (both interfaces
 *     are optional and null-guarded in the core).
 *   - GetPanelFeatureFactoryIntf() returns an inert factory. The rounded
 *     corner feature it feeds stays disabled by default on this panel.
 */

#include <dlfcn.h>
#include <memory>
#include <new>

#include <cstddef>

#include <dpps_control_interface.h>
#include <private/extension_interface.h>
#include <private/panel_feature_factory_intf.h>
#include <private/resource_interface.h>
#include <private/strategy_interface.h>
#include <utils/constants.h>
#include <utils/rect.h>

namespace sdm {

namespace {

// ResourceDefault lives inside libsdmcore, which is a legacy Android.mk
// module and cannot be linked from this soong module. Its factory entry
// points are exported symbols, resolved here at runtime. libsdmcore is
// always already loaded - it is what dlopen()s this library.
using CreateResourceDefaultFn =
    DisplayError (*)(const HWResourceInfo &, ResourceInterface **);
using DestroyResourceDefaultFn = DisplayError (*)(ResourceInterface *);

CreateResourceDefaultFn GetCreateResourceDefault() {
  static auto fn = reinterpret_cast<CreateResourceDefaultFn>(dlsym(
      RTLD_DEFAULT,
      "_ZN3sdm15ResourceDefault21CreateResourceDefaultERKNS_"
      "14HWResourceInfoEPPNS_17ResourceInterfaceE"));
  return fn;
}

DestroyResourceDefaultFn GetDestroyResourceDefault() {
  static auto fn = reinterpret_cast<DestroyResourceDefaultFn>(dlsym(
      RTLD_DEFAULT,
      "_ZN3sdm15ResourceDefault22DestroyResourceDefaultEPNS_"
      "17ResourceInterfaceE"));
  return fn;
}

// Minimal strategy: composite every application layer on the GPU into the
// framebuffer target and program that single target to the panel, matching
// the core's built-in fallback in Strategy::GetNextStrategy().
class GpuOnlyStrategy : public StrategyInterface {
 public:
  GpuOnlyStrategy(const HWPanelInfo &panel_info,
                  const HWMixerAttributes &mixer_attributes,
                  const DisplayConfigVariableInfo &fb_config)
      : hw_panel_info_(panel_info),
        mixer_attributes_(mixer_attributes),
        fb_config_(fb_config) {}

  DisplayError Start(HWLayersInfo *hw_layers_info, uint32_t *max_attempts) override {
    hw_layers_info_ = hw_layers_info;
    // Per-frame vectors are otherwise only reset on Flush()/PowerOff().
    hw_layers_info->hw_layers.clear();
    hw_layers_info->index.clear();
    hw_layers_info->roi_index.clear();
    hw_layers_info->layer_exts.clear();
    // BuildLayerStackStats() appends per frame without clearing; keep the
    // entries of the current frame only.
    while (hw_layers_info->wide_color_primaries.size() >
           hw_layers_info->app_layer_count) {
      hw_layers_info->wide_color_primaries.erase(
          hw_layers_info->wide_color_primaries.begin());
    }
    *max_attempts = 1;
    return kErrorNone;
  }

  DisplayError GetNextStrategy(StrategyConstraints *constraints) override {
    (void)constraints;
    if (disable_gpu_comp_ || !hw_layers_info_->gpu_target_index) {
      return kErrorNotSupported;
    }

    LayerStack *layer_stack = hw_layers_info_->stack;
    for (uint32_t i = 0; i < hw_layers_info_->app_layer_count; i++) {
      layer_stack->layers.at(i)->composition = kCompositionGPU;
      layer_stack->layers.at(i)->request.flags.request_flags = 0;
    }

    // When mixer resolution and panel resolutions are same (1600x2560) and
    // FB resolution is 1080x1920 FB_Target destination coordinates (mapped
    // to FB resolution 1080x1920) need to be mapped to destination
    // coordinates of mixer resolution (1600x2560).
    Layer *gpu_target_layer =
        layer_stack->layers.at(hw_layers_info_->gpu_target_index);
    float layer_mixer_width = FLOAT(mixer_attributes_.width);
    float layer_mixer_height = FLOAT(mixer_attributes_.height);
    float fb_width = FLOAT(fb_config_.x_pixels);
    float fb_height = FLOAT(fb_config_.y_pixels);
    LayerRect src_domain = {0.0f, 0.0f, fb_width, fb_height};
    LayerRect dst_domain = {0.0f, 0.0f, layer_mixer_width, layer_mixer_height};

    Layer layer = *gpu_target_layer;
    hw_layers_info_->index.push_back(hw_layers_info_->gpu_target_index);
    hw_layers_info_->roi_index.push_back(0);
    layer.transform.flip_horizontal ^=
        hw_panel_info_.panel_orientation.flip_horizontal;
    layer.transform.flip_vertical ^=
        hw_panel_info_.panel_orientation.flip_vertical;
    // Flip rect to match transform.
    TransformHV(src_domain, layer.dst_rect, layer.transform, &layer.dst_rect);
    // Scale to mixer resolution.
    MapRect(src_domain, dst_domain, layer.dst_rect, &layer.dst_rect);
    hw_layers_info_->hw_layers.push_back(layer);

    // Nothing in the open-source drop computes the per-frame bandwidth/clock
    // votes - on Samsung this is done by the proprietary resource extension
    // through libdisplayqos. Without votes every commit programs
    // core_clk/buses to 0, the SDE pipe starves and the encoder underflows
    // (visible as static noise after every resume). Program the votes the
    // stock stack uses for the 1600x2560@60 panel.
    static_assert(offsetof(HWLayers, info) == 0,
                  "HWLayers::info must be the first member");
    HWLayers *hw_layers =
        reinterpret_cast<HWLayers *>(hw_layers_info_);
    hw_layers->qos_data.clock_hz = 266313600;
    hw_layers->qos_data.core_ab_bps = 2032878250;
    hw_layers->qos_data.llcc_ab_bps = 2032878250;
    hw_layers->qos_data.dram_ab_bps = 2032878250;
    hw_layers->qos_data.dram_ib_bps = 1600000000;

    return kErrorNone;
  }

  DisplayError Stop() override { return kErrorNone; }

  DisplayError Reconfigure(const HWPanelInfo &hw_panel_info,
                           const HWResourceInfo &hw_res_info,
                           const HWMixerAttributes &mixer_attributes,
                           const DisplayConfigVariableInfo &fb_config) override {
    (void)hw_res_info;
    hw_panel_info_ = hw_panel_info;
    mixer_attributes_ = mixer_attributes;
    fb_config_ = fb_config;
    return kErrorNone;
  }

  DisplayError SetCompositionState(LayerComposition composition_type,
                                   bool enable) override {
    if (composition_type == kCompositionGPU) {
      disable_gpu_comp_ = !enable;
    }
    return kErrorNone;
  }

  DisplayError Purge() override { return kErrorNone; }

  DisplayError SetIdleTimeoutMs(uint32_t active_ms,
                                uint32_t inactive_ms) override {
    (void)active_ms;
    (void)inactive_ms;
    return kErrorNone;
  }

  DisplayError SetColorModesInfo(
      const std::vector<PrimariesTransfer> &colormodes_cs) override {
    (void)colormodes_cs;
    return kErrorNone;
  }

  DisplayError SetBlendSpace(const PrimariesTransfer &blend_space) override {
    (void)blend_space;
    return kErrorNone;
  }

  bool CanSkipValidate(bool *needs_buffer_swap) override {
    *needs_buffer_swap = false;
    return false;
  }

  DisplayError SwapBuffers() override { return kErrorNone; }

 private:
  HWLayersInfo *hw_layers_info_ = nullptr;
  HWPanelInfo hw_panel_info_ = {};
  HWMixerAttributes mixer_attributes_ = {};
  DisplayConfigVariableInfo fb_config_ = {};
  bool disable_gpu_comp_ = false;
};

class StubExtension : public ExtensionInterface {
 public:
  DisplayError CreatePartialUpdate(
      int32_t display_id, DisplayType type,
      const HWResourceInfo &hw_resource_info,
      const HWPanelInfo &hw_panel_info,
      const HWMixerAttributes &mixer_attributes,
      const HWDisplayAttributes &display_attributes,
      const DisplayConfigVariableInfo &fb_config,
      PartialUpdateInterface **interface) override {
    (void)display_id;
    (void)type;
    (void)hw_resource_info;
    (void)hw_panel_info;
    (void)mixer_attributes;
    (void)display_attributes;
    (void)fb_config;
    *interface = nullptr;
    return kErrorNone;
  }

  DisplayError DestroyPartialUpdate(PartialUpdateInterface *interface) override {
    (void)interface;
    return kErrorNone;
  }

  DisplayError CreateStrategyExtn(
      int32_t display_id, DisplayType type, BufferAllocator *buffer_allocator,
      const HWResourceInfo &hw_resource_info,
      const HWPanelInfo &hw_panel_info,
      const HWMixerAttributes &mixer_attributes,
      const DisplayConfigVariableInfo &fb_config,
      StrategyInterface **interface) override {
    (void)display_id;
    (void)type;
    (void)buffer_allocator;
    (void)hw_resource_info;
    *interface = new (std::nothrow)
        GpuOnlyStrategy(hw_panel_info, mixer_attributes, fb_config);
    return *interface ? kErrorNone : kErrorResources;
  }

  DisplayError DestroyStrategyExtn(StrategyInterface *interface) override {
    delete static_cast<GpuOnlyStrategy *>(interface);
    return kErrorNone;
  }

  DisplayError CreateResourceExtn(
      const HWResourceInfo &hw_resource_info,
      BufferAllocator *buffer_allocator,
      ResourceInterface **interface) override {
    (void)buffer_allocator;
    auto create = GetCreateResourceDefault();
    if (!create) {
      return kErrorNotSupported;
    }
    return create(hw_resource_info, interface);
  }

  DisplayError DestroyResourceExtn(ResourceInterface *interface) override {
    auto destroy = GetDestroyResourceDefault();
    if (!destroy) {
      return kErrorNotSupported;
    }
    return destroy(interface);
  }

  DisplayError CreateDppsControlExtn(
      DppsControlInterface **dpps_control_interface,
      SocketHandler *socket_handler) override {
    (void)socket_handler;
    *dpps_control_interface = nullptr;
    return kErrorNone;
  }

  DisplayError DestroyDppsControlExtn(DppsControlInterface *interface) override {
    (void)interface;
    return kErrorNone;
  }
};

class StubPanelFeatureFactory : public PanelFeatureFactoryIntf {
 public:
  std::unique_ptr<RCIntf> CreateRCIntf(
      const RCInputConfig &input_cfg,
      PanelFeaturePropertyIntf *prop_intf) override {
    (void)input_cfg;
    (void)prop_intf;
    return nullptr;
  }
};

}  // namespace

}  // namespace sdm

extern "C" sdm::DisplayError CreateExtensionInterface(
    uint16_t version, sdm::ExtensionInterface **interface) {
  (void)version;
  *interface = new (std::nothrow) sdm::StubExtension();
  return *interface ? sdm::kErrorNone : sdm::kErrorResources;
}

extern "C" sdm::DisplayError DestroyExtensionInterface(
    sdm::ExtensionInterface *interface) {
  delete static_cast<sdm::StubExtension *>(interface);
  return sdm::kErrorNone;
}

extern "C" sdm::PanelFeatureFactoryIntf *GetPanelFeatureFactoryIntf() {
  static sdm::StubPanelFeatureFactory factory;
  return &factory;
}
