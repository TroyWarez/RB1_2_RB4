#include "plugin_common.h"
#include "Common.h"
#include <Patcher.h>
#include "config.h"
#include "pad.h"

attr_public const char* g_pluginName = "RB1_2_RB4";
attr_public const char* g_pluginDesc = "Turn your Rockband 1 guitar into a Rockband 4 guitar to enable navigation of the playstation menu and persistent sign in of your PSN profile.";
attr_public const char* g_pluginAuth = "TroyWarez";
attr_public u32 g_pluginVersion = 0x00000100;  // 1.00

HOOK_INIT(scePadRead);
HOOK_INIT(scePadReadState);
HOOK_INIT(scePadOpenExt);
HOOK_INIT(scePadGetExtControllerInformation);


Patcher* scePadReadExtPatcher;
Patcher* scePadReadStateExtPatcher;
Patcher* scePadOpenExtPatcher;
Patcher* scePadGetExtControllerInformationPatcher;

#define PLUGIN_DEFAULT_SECTION "default"

#define PS3_STRATOCASTER_VENDOR_ID 0x12BA
#define PS3_STRATOCASTER_PRODUCT_ID 0x0200
/*
Hooked funcs for rb1
 sceUsbdGetDeviceList
 sceUsbdGetDeviceDescriptor
*/

#define PS4_STRATOCASTER_VENDOR_ID 0x0738
#define PS4_STRATOCASTER_PRODUCT_ID 0x8261
#define STR_MANUFACTURER "Mad Catz, Inc."
#define STR_PRODUCT "Mad Catz Guitar for RB4"
/*
Hooked funcs for rb4
 scePadOpenExt
 scePadGetExtControllerInformation
*/
int custom_touchpad(int32_t handle, ScePadData* pData) {
    if (g_enableCustomTouchPad) {
        if (pData->buttons & SCE_PAD_BUTTON_TOUCH_PAD) {
            // clear touch pad button flag
            pData->buttons &= ~SCE_PAD_BUTTON_TOUCH_PAD;

            if (pData->touchData.touch[0].x < 960 && pData->touchData.touch[0].y < 471) {
                pData->buttons |= buttonMapping[TOUCH_L1];
            }

            if (pData->touchData.touch[0].x > 960 && pData->touchData.touch[0].y < 471) {
                pData->buttons |= buttonMapping[TOUCH_R1];
            }

            if (pData->touchData.touch[0].x < 960 && pData->touchData.touch[0].y > 471) {
                pData->buttons |= buttonMapping[TOUCH_L2];
            }

            if (pData->touchData.touch[0].x > 960 && pData->touchData.touch[0].y > 471) {
                pData->buttons |= buttonMapping[TOUCH_R2];
            }
        }
    }
    return 0;
}

int custom_button(int32_t handle, ScePadData* pData) {
    if (g_enableCustomButton) {
        uint32_t buttons = 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_L3) ? buttonMapping[BUTTON_L3] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_R3) ? buttonMapping[BUTTON_R3] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_OPTIONS) ? buttonMapping[BUTTON_OPTIONS] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_UP) ? buttonMapping[BUTTON_UP] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_RIGHT) ? buttonMapping[BUTTON_RIGHT] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_DOWN) ? buttonMapping[BUTTON_DOWN] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_LEFT) ? buttonMapping[BUTTON_LEFT] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_L2) ? buttonMapping[BUTTON_L2] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_R2) ? buttonMapping[BUTTON_R2] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_L1) ? buttonMapping[BUTTON_L1] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_R1) ? buttonMapping[BUTTON_R1] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_TRIANGLE) ? buttonMapping[BUTTON_TRIANGLE] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_CIRCLE) ? buttonMapping[BUTTON_CIRCLE] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_CROSS) ? buttonMapping[BUTTON_CROSS] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_SQUARE) ? buttonMapping[BUTTON_SQUARE] : 0;
        buttons |= (pData->buttons & SCE_PAD_BUTTON_INTERCEPTED) ? buttonMapping[BUTTON_INTERCEPTED] : 0;

        if (g_enableCustomTouchPad) {
            buttons |= (pData->buttons & SCE_PAD_BUTTON_TOUCH_PAD) ? SCE_PAD_BUTTON_TOUCH_PAD : 0;
        } else {
            buttons |= (pData->buttons & SCE_PAD_BUTTON_TOUCH_PAD) ? buttonMapping[BUTTON_TOUCH_PAD] : 0;
        }

        pData->buttons = buttons;
    }
    return 0;
}

int32_t scePadRead_hook(int32_t handle, ScePadData* pData, int32_t num) {
    int ret = 0;
    ret = scePadReadExt(handle, pData, num);

    if (ret <= 0) {
        return ret;
    }
    for (int i = 0; i < ret; i++) {
        deadzone_apply(&pData[i]);
        custom_button(handle, &pData[i]);
        custom_touchpad(handle, &pData[i]);
    }
    return ret;
}

int32_t scePadReadState_hook(int32_t handle, ScePadData* pData) {
    int ret = 0;
    ret = scePadReadStateExt(handle, pData);

    if (ret) {
        return ret;
    }
    deadzone_apply(pData);
    custom_button(handle, pData);
    custom_touchpad(handle, pData);
    return ret;
}

int32_t load_config(ini_table_s* table, const char* section_name) {
    ini_table_get_entry_as_bool(table, section_name, "enableDeadZone", &g_enableDeadZone);

    if (g_enableDeadZone) {
        ini_table_get_entry_as_int(table, section_name, "DeadZoneLeft", &g_deadZoneLeft);
        ini_table_get_entry_as_int(table, section_name, "DeadZoneRight", &g_deadZoneRight);
    }

    ini_table_get_entry_as_bool(table, section_name, "enableCustomTouchPad", &g_enableCustomTouchPad);

    if (g_enableCustomTouchPad) {
        ini_table_get_entry_as_scePadButton(table, section_name, "TOUCH_L1", &buttonMapping[TOUCH_L1]);
        ini_table_get_entry_as_scePadButton(table, section_name, "TOUCH_R1", &buttonMapping[TOUCH_R1]);
        ini_table_get_entry_as_scePadButton(table, section_name, "TOUCH_L2", &buttonMapping[TOUCH_L2]);
        ini_table_get_entry_as_scePadButton(table, section_name, "TOUCH_R2", &buttonMapping[TOUCH_R2]);
    }

    ini_table_get_entry_as_bool(table, section_name, "enableCustomButton", &g_enableCustomButton);

    if (g_enableCustomButton) {
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_L3", &buttonMapping[BUTTON_L3]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_R3", &buttonMapping[BUTTON_R3]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_OPTIONS", &buttonMapping[BUTTON_OPTIONS]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_UP", &buttonMapping[BUTTON_UP]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_RIGHT", &buttonMapping[BUTTON_RIGHT]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_DOWN", &buttonMapping[BUTTON_DOWN]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_LEFT", &buttonMapping[BUTTON_LEFT]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_L2", &buttonMapping[BUTTON_L2]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_R2", &buttonMapping[BUTTON_R2]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_L1", &buttonMapping[BUTTON_L1]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_R1", &buttonMapping[BUTTON_R1]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_TRIANGLE", &buttonMapping[BUTTON_TRIANGLE]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_CIRCLE", &buttonMapping[BUTTON_CIRCLE]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_CROSS", &buttonMapping[BUTTON_CROSS]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_SQUARE", &buttonMapping[BUTTON_SQUARE]);
        ini_table_get_entry_as_scePadButton(table, section_name, "BUTTON_TOUCH_PAD", &buttonMapping[BUTTON_TOUCH_PAD]);
    }

    ini_table_get_entry_as_viration_intensity(table, section_name, "VirationIntensity", &g_virationIntensity);
    return 0;
}

s32 attr_public plugin_load(s32 argc, const char* argv[]) {
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    final_printf("[GoldHEN] Plugin Author(s): %s\n", g_pluginAuth);
    boot_ver();

    char module[256];
    snprintf(module, 256, "/%s/common/lib/%s", sceKernelGetFsSandboxRandomWord(), "libScePad.sprx");
    int h = 0;
    sys_dynlib_load_prx(module, &h);

    char module2[256];
    snprintf(module, 256, "/%s/common/lib/%s", sceKernelGetFsSandboxRandomWord(), "libSceUsbd.prx");
    int h2 = 0;
    sys_dynlib_load_prx(module2, &h2);

    scePadReadExtPatcher = (Patcher*)malloc(sizeof(Patcher));
    scePadReadStateExtPatcher = (Patcher*)malloc(sizeof(Patcher));
    Patcher_Construct(scePadReadExtPatcher);
    Patcher_Construct(scePadReadStateExtPatcher);

    uint8_t xor_ecx_ecx[5] = {0x31, 0xC9, 0x90, 0x90, 0x90};
    Patcher_Install_Patch(scePadReadExtPatcher, (uint64_t)scePadReadExt, xor_ecx_ecx, sizeof(xor_ecx_ecx));

    uint8_t xor_edx_edx[5] = {0x31, 0xD2, 0x90, 0x90, 0x90};
    Patcher_Install_Patch(scePadReadStateExtPatcher, (uint64_t)scePadReadStateExt, xor_edx_edx, sizeof(xor_edx_edx));

    HOOK32(scePadRead);
    HOOK32(scePadReadState);

    // defaults value
    g_enableDeadZone = true;

    g_deadZoneLeft = 0xd;
    g_deadZoneRight = 0xd;

    g_enableCustomTouchPad = false;
    g_enableCustomButton = false;
    g_virationIntensity = PAD_VIRATION_INTENSITY_STRONG;

    buttonMapping = (uint32_t*)malloc(BUTTON_MAPPING_MAX * sizeof(uint32_t));

    memset(buttonMapping, 0, BUTTON_MAPPING_MAX * sizeof(uint32_t));

    buttonMapping[TOUCH_L1] = SCE_PAD_BUTTON_TOUCH_PAD;
    buttonMapping[TOUCH_R1] = SCE_PAD_BUTTON_TOUCH_PAD;
    buttonMapping[TOUCH_L2] = SCE_PAD_BUTTON_TOUCH_PAD;
    buttonMapping[TOUCH_R2] = SCE_PAD_BUTTON_TOUCH_PAD;

    buttonMapping[BUTTON_L3] = SCE_PAD_BUTTON_L3;
    buttonMapping[BUTTON_R3] = SCE_PAD_BUTTON_R3;
    buttonMapping[BUTTON_OPTIONS] = SCE_PAD_BUTTON_OPTIONS;
    buttonMapping[BUTTON_UP] = SCE_PAD_BUTTON_UP;
    buttonMapping[BUTTON_RIGHT] = SCE_PAD_BUTTON_RIGHT;
    buttonMapping[BUTTON_DOWN] = SCE_PAD_BUTTON_DOWN;
    buttonMapping[BUTTON_LEFT] = SCE_PAD_BUTTON_LEFT;
    buttonMapping[BUTTON_L2] = SCE_PAD_BUTTON_L2;
    buttonMapping[BUTTON_R2] = SCE_PAD_BUTTON_R2;
    buttonMapping[BUTTON_L1] = SCE_PAD_BUTTON_L1;
    buttonMapping[BUTTON_R1] = SCE_PAD_BUTTON_R1;
    buttonMapping[BUTTON_TRIANGLE] = SCE_PAD_BUTTON_TRIANGLE;
    buttonMapping[BUTTON_CIRCLE] = SCE_PAD_BUTTON_CIRCLE;
    buttonMapping[BUTTON_CROSS] = SCE_PAD_BUTTON_CROSS;
    buttonMapping[BUTTON_SQUARE] = SCE_PAD_BUTTON_SQUARE;
    buttonMapping[BUTTON_TOUCH_PAD] = SCE_PAD_BUTTON_TOUCH_PAD;
    buttonMapping[BUTTON_INTERCEPTED] = SCE_PAD_BUTTON_INTERCEPTED;

    // load config
    struct proc_info procInfo;
    if (sys_sdk_proc_info(&procInfo) == 0) {
        print_proc_info();
    } else {
        final_printf("failed to initialise\n");
        return -1;
    }

    if (!file_exists(PLUGIN_CONFIG_PATH)) {
        final_printf("Not found gamepad.ini config file\n");
        return 0;
    }

    ini_table_s* config = ini_table_create();
    if (config == NULL) {
        final_printf("Config parser failed to initialise\n");
        return -1;
    }

    if (!ini_table_read_from_file(config, PLUGIN_CONFIG_PATH)) {
        final_printf("Config parser failed to parse config: %s\n", PLUGIN_CONFIG_PATH);
        return -1;
    }

    final_printf("Section is TitleID [%s]\n", procInfo.titleid);

    for (uint16_t i = 0; i < config->size; i++) {
        ini_section_s* section = &config->section[i];

        if (section == NULL) continue;

        if (strcmp(section->name, PLUGIN_DEFAULT_SECTION) == 0) {
            final_printf("Section [%s] is default\n", section->name);
            load_config(config, section->name);
        } else if (strcmp(section->name, procInfo.titleid) == 0) {
            final_printf("Section is TitleID [%s]\n", procInfo.titleid);
            load_config(config, section->name);
        }
    }

    if (config != NULL) {
        ini_table_destroy(config);
    }

    // check config value
    if (g_deadZoneLeft > 0xFF && g_deadZoneLeft < 0) {
        final_printf("deadZoneLeft config error, restore default value");
        g_deadZoneLeft = 0xd;
    }

    if (g_deadZoneRight > 0xFF && g_deadZoneRight < 0) {
        final_printf("deadZoneRight config error, restore default value");
        g_deadZoneRight = 0xd;
    }

    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char* argv[]) {
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    UNHOOK(scePadRead);
    UNHOOK(scePadReadState);

    Patcher_Destroy(scePadReadExtPatcher);
    Patcher_Destroy(scePadReadStateExtPatcher);
    free(scePadReadExtPatcher);
    free(scePadReadStateExtPatcher);
    free(buttonMapping);

    return 0;
}

s32 attr_module_hidden module_start(s64 argc, const void *args)
{
    return 0;
}

s32 attr_module_hidden module_stop(s64 argc, const void *args)
{
    return 0;
}
