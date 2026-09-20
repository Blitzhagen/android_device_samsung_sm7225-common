# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0

import common


def AddImage(info, dir, basename, dest):
    path = dir + "/" + basename
    data = info.input_zip.read(path)
    f = common.File(basename, data)
    f.AddToZip(info.output_zip)
    info.script.AppendExtra('package_extract_file("%s", "%s");' % (basename, dest))


def FullOTA_InstallEnd(info):
    AddImage(info, "IMAGES", "dtbo.img", "/dev/block/bootdevice/by-name/dtbo")
    AddImage(info, "IMAGES", "vbmeta.img", "/dev/block/bootdevice/by-name/vbmeta")


def IncrementalOTA_InstallEnd(info):
    FullOTA_InstallEnd(info)
