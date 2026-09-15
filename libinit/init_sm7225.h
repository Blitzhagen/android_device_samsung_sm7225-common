#ifndef INIT_SEC_H
#define INIT_SEC_H

#include <string.h>

enum device_variant {
    VARIANT_M236B = 0,
    VARIANT_E236B,
    VARIANT_M236L,
    VARIANT_M236Q,
    VARIANT_A426B,
    VARIANT_A426N,
    VARIANT_A426U,
    VARIANT_M426B,
    VARIANT_A4260,
    VARIANT_T736B,
    VARIANT_T736N,
    VARIANT_T738U,
    VARIANT_A526B,
    VARIANT_A526U,
    VARIANT_A526U1,
    VARIANT_A526W,
    VARIANT_E5260,
    VARIANT_MAX
};

typedef struct {
    std::string model;
    std::string codename;
} variant;

static const variant international_models_m23 = {
    .model = "SM-M236B",
    .codename = "m23xq"
};

static const variant india_models_m23 = {
    .model = "SM-E236B",
    .codename = "m23xq"
};

static const variant korea_models_m23 = {
    .model = "SM-M236L",
    .codename = "m23xq"
};

static const variant japan_models_m23 = {
    .model = "SM-M236Q",
    .codename = "m23xq"
};

static const variant international_models_a42 = {
    .model = "SM-A426B",
    .codename = "a42xq"
};

static const variant korea_models_a42 = {
    .model = "SM-A426N",
    .codename = "a42xq"
};

static const variant america_models_a42 = {
    .model = "SM-A426U",
    .codename = "a42xq"
};

static const variant india_models_a42 = {
    .model = "SM-M426B",
    .codename = "a42xq"
};

static const variant hongkong_models_a42 = {
    .model = "SM-A4260",
    .codename = "a42xq"
};

static const variant international_models_gts7xllite = {
    .model = "SM-T736B",
    .codename = "gts7xllite"
};

static const variant korea_models_gts7xllite = {
    .model = "SM-T736N",
    .codename = "gts7xllite"
};

static const variant america_models_gts7xllite = {
    .model = "SM-T738U",
    .codename = "gts7xllite"
};

static const variant international_models_a52 = {
    .model = "SM-A526B",
    .codename = "a52xq"
};

static const variant america_models_a52 = {
    .model = "SM-A526U",
    .codename = "a52xq"
};

static const variant america2_models_a52 = {
    .model = "SM-A526U1",
    .codename = "a52xq"
};

static const variant canada_models_a52 = {
    .model = "SM-A526W",
    .codename = "a52xq"
};

static const variant china_models_a52 = {
    .model = "SM-E5260",
    .codename = "a52xq"
};

static const variant *all_variants[VARIANT_MAX] = {
    &international_models_m23,
    &india_models_m23,
    &korea_models_m23,
    &japan_models_m23,
    &international_models_a42,
    &korea_models_a42,
    &america_models_a42,
    &india_models_a42,
    &hongkong_models_a42,
    &international_models_gts7xllite,
    &korea_models_gts7xllite,
    &america_models_gts7xllite,
    &international_models_a52,
    &america_models_a52,
    &america2_models_a52,
    &canada_models_a52,
    &china_models_a52,
};

#endif // INIT_SEC_H
