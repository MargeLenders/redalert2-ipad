#ifndef IOS_DISPLAY_BRIDGE_H
#define IOS_DISPLAY_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize iOS display monitoring (hinge interaction, safe areas) */
void ios_display_init(void *sdl_window);

/* Query whether the device is currently a folded/closed dual-screen device (iPhone Duo) */
int ios_display_is_duo_closed(void);

/* Query safe area insets (in physical pixels) */
void ios_display_get_safe_area_pixels(int renderer_w, int renderer_h,
                                     int *top, int *bottom, int *left, int *right);

/* Query optimal game resolution for current screen aspect ratio (base height 720) */
void ios_display_get_target_resolution(int *out_w, int *out_h);

/* Update hinge status or refresh overlay */
void ios_display_poll(void);

/* Save current UIWindow screenshot directly to disk */
void ios_display_save_screenshot(const char *path);

/* Virtual on-screen keyboard control */
void ios_display_show_keyboard(int x, int y, int w, int h);
void ios_display_hide_keyboard(void);

#ifdef __cplusplus
}
#endif

#endif /* IOS_DISPLAY_BRIDGE_H */
