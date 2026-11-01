#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <SDL2/SDL.h>
#import <SDL2/SDL_syswm.h>
#import "ios_display_bridge.h"

#if __has_include(<UIKit/UIHinge.h>)
#import <UIKit/UIHinge.h>
#import <UIKit/UIHingeInteraction.h>
#endif

/* Tactical military themed overlay for iPhone Duo when folded closed */
@interface DuoFoldNoticeView : UIView
@property (nonatomic, strong) UIView *headerBar;
@property (nonatomic, strong) UILabel *titleLabel;
@property (nonatomic, strong) UILabel *messageLabel;
@property (nonatomic, strong) UILabel *statusLabel;
@property (nonatomic, strong) UIView *cardView;
@end

@implementation DuoFoldNoticeView

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.backgroundColor = [UIColor colorWithRed:0.07 green:0.08 blue:0.10 alpha:1.0];
        self.userInteractionEnabled = YES;

        // Red tactical header bar
        self.headerBar = [[UIView alloc] initWithFrame:CGRectMake(0, 0, frame.size.width, 50)];
        self.headerBar.backgroundColor = [UIColor colorWithRed:0.78 green:0.12 blue:0.12 alpha:1.0];
        self.headerBar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;
        [self addSubview:self.headerBar];

        UILabel *headerTitle = [[UILabel alloc] initWithFrame:CGRectMake(10, 10, frame.size.width - 20, 30)];
        headerTitle.text = @"\u2605 ALERTA ROJA 2 // COMANDO T\u00C1CTICO \u2605";
        headerTitle.textColor = [UIColor whiteColor];
        headerTitle.font = [UIFont fontWithName:@"Courier-Bold" size:15] ?: [UIFont boldSystemFontOfSize:15];
        headerTitle.textAlignment = NSTextAlignmentCenter;
        headerTitle.autoresizingMask = UIViewAutoresizingFlexibleWidth;
        [self.headerBar addSubview:headerTitle];

        // Central tactical briefing card
        CGFloat cardW = MIN(frame.size.width - 40, 420);
        CGFloat cardH = 320;
        self.cardView = [[UIView alloc] initWithFrame:CGRectMake((frame.size.width - cardW) / 2,
                                                                 (frame.size.height - cardH) / 2,
                                                                 cardW, cardH)];
        self.cardView.backgroundColor = [UIColor colorWithRed:0.12 green:0.13 blue:0.16 alpha:0.95];
        self.cardView.layer.borderColor = [UIColor colorWithRed:0.95 green:0.75 blue:0.20 alpha:0.9].CGColor;
        self.cardView.layer.borderWidth = 2.0;
        self.cardView.layer.cornerRadius = 10.0;
        [self addSubview:self.cardView];

        // Tactical icon (dual-screen schematic)
        UILabel *iconLabel = [[UILabel alloc] initWithFrame:CGRectMake(20, 20, cardW - 40, 50)];
        iconLabel.text = @"\u25C8 \u25C8 \u25C8";
        iconLabel.textColor = [UIColor colorWithRed:0.95 green:0.75 blue:0.20 alpha:1.0];
        iconLabel.font = [UIFont systemFontOfSize:32 weight:UIFontWeightHeavy];
        iconLabel.textAlignment = NSTextAlignmentCenter;
        [self.cardView addSubview:iconLabel];

        // Main Title
        self.titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(20, 75, cardW - 40, 32)];
        self.titleLabel.text = @"DESPLIEGUE EL DISPOSITIVO";
        self.titleLabel.textColor = [UIColor colorWithRed:1.0 green:0.85 blue:0.25 alpha:1.0];
        self.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-CondensedBlack" size:22] ?: [UIFont boldSystemFontOfSize:22];
        self.titleLabel.textAlignment = NSTextAlignmentCenter;
        [self.cardView addSubview:self.titleLabel];

        // Military briefing message
        self.messageLabel = [[UILabel alloc] initWithFrame:CGRectMake(24, 115, cardW - 48, 120)];
        self.messageLabel.text = @"Comandante: Se requiere la pantalla interior del iPhone Duo para desplegar el centro de mando t\u00E1ctico.\n\nPor favor, abra el iPhone Duo para iniciar el combate.";
        self.messageLabel.textColor = [UIColor colorWithRed:0.90 green:0.92 blue:0.95 alpha:1.0];
        self.messageLabel.font = [UIFont systemFontOfSize:15 weight:UIFontWeightMedium];
        self.messageLabel.textAlignment = NSTextAlignmentCenter;
        self.messageLabel.numberOfLines = 0;
        [self.cardView addSubview:self.messageLabel];

        // Bottom status badge
        self.statusLabel = [[UILabel alloc] initWithFrame:CGRectMake(20, 255, cardW - 40, 35)];
        self.statusLabel.text = @"[ ESTADO: PANTALLA EXTERIOR INACTIVA ]";
        self.statusLabel.textColor = [UIColor colorWithRed:1.0 green:0.3 blue:0.3 alpha:1.0];
        self.statusLabel.font = [UIFont fontWithName:@"Courier-Bold" size:13] ?: [UIFont boldSystemFontOfSize:13];
        self.statusLabel.textAlignment = NSTextAlignmentCenter;
        self.statusLabel.backgroundColor = [UIColor colorWithRed:0.25 green:0.05 blue:0.05 alpha:0.6];
        self.statusLabel.layer.cornerRadius = 6.0;
        self.statusLabel.layer.masksToBounds = YES;
        [self.cardView addSubview:self.statusLabel];

        // Pulsing animation on status label
        [UIView animateWithDuration:1.0
                              delay:0.0
                            options:UIViewAnimationOptionRepeat | UIViewAnimationOptionAutoreverse | UIViewAnimationOptionAllowUserInteraction
                         animations:^{
            self.statusLabel.alpha = 0.35;
        } completion:nil];
    }
    return self;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGRect b = self.bounds;
    self.headerBar.frame = CGRectMake(0, 0, b.size.width, 50);
    CGFloat cardW = MIN(b.size.width - 40, 420);
    CGFloat cardH = MIN(b.size.height - 60, 320);
    self.cardView.frame = CGRectMake((b.size.width - cardW) / 2, (b.size.height - cardH) / 2, cardW, cardH);
}

@end

static int g_is_duo_closed = 0;
static DuoFoldNoticeView *g_notice_view = nil;
static id g_hinge_interaction = nil;
static UIWindow *g_sdl_window = nil;

static UIWindow *get_key_window(void) {
    if (g_sdl_window) return g_sdl_window;
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            UIWindowScene *ws = (UIWindowScene *)scene;
            for (UIWindow *w in ws.windows) {
                if (w.isKeyWindow) return w;
            }
            if (ws.windows.count > 0) return ws.windows.firstObject;
        }
    }
    return nil;
}

static void update_notice_visibility(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *win = get_key_window();
        fprintf(stderr, "[ios_display] update_notice_visibility: win=%p bounds=(%.1f, %.1f) is_duo_closed=%d\n",
                (void *)win, win ? win.bounds.size.width : 0.0, win ? win.bounds.size.height : 0.0, g_is_duo_closed);
        if (g_is_duo_closed) {
            if (win) {
                if (!g_notice_view) {
                    g_notice_view = [[DuoFoldNoticeView alloc] initWithFrame:win.bounds];
                    g_notice_view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
                    g_notice_view.layer.zPosition = 1000.0f;
                } else {
                    g_notice_view.frame = win.bounds;
                }
                if (g_notice_view.superview != win) {
                    [g_notice_view removeFromSuperview];
                    [win addSubview:g_notice_view];
                }
                [win bringSubviewToFront:g_notice_view];
                g_notice_view.alpha = 1.0;
                g_notice_view.hidden = NO;
                [g_notice_view setNeedsLayout];
                [g_notice_view layoutIfNeeded];
                fprintf(stderr, "[ios_display] DuoFoldNoticeView presented on window %p\n", (void *)win);
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.4 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    const char *shot_path = getenv("RA2_SCREENSHOT_PATH");
                    if (shot_path && shot_path[0]) {
                        ios_display_save_screenshot(shot_path);
                    }
                });
            }
        } else {
            if (g_notice_view && g_notice_view.superview) {
                [UIView animateWithDuration:0.3 animations:^{
                    g_notice_view.alpha = 0.0;
                } completion:^(BOOL finished) {
                    g_notice_view.hidden = YES;
                    [g_notice_view removeFromSuperview];
                }];
            }
        }
    });
}

void ios_display_init(void *sdl_window) {
    if (sdl_window) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo((SDL_Window *)sdl_window, &wmInfo)) {
            g_sdl_window = wmInfo.info.uikit.window;
            fprintf(stderr, "[ios_display] Captured SDL UIWindow: %p\n", (void *)g_sdl_window);
        }
    }

    /* Check simulation override environment variable */
    const char *open_env = getenv("RA2_FORCE_DUO_OPEN");
    if (open_env && (!strcmp(open_env, "1") || !strcasecmp(open_env, "yes") || !strcasecmp(open_env, "true"))) {
        fprintf(stderr, "[ios_display] RA2_FORCE_DUO_OPEN active: forcing unfolded iPhone Duo\n");
        g_is_duo_closed = 0;
        update_notice_visibility();
        return;
    }

    const char *sim_env = getenv("RA2_SIMULATE_DUO_CLOSED");
    if (sim_env && (!strcmp(sim_env, "1") || !strcasecmp(sim_env, "yes") || !strcasecmp(sim_env, "true"))) {
        fprintf(stderr, "[ios_display] RA2_SIMULATE_DUO_CLOSED active: simulating closed iPhone Duo\n");
        g_is_duo_closed = 1;
        update_notice_visibility();
        return;
    }

    /* Attach UIHingeInteraction if supported on iOS 27.1+ */
    if (@available(iOS 27.1, *)) {
        Class hingeInteractionClass = NSClassFromString(@"UIHingeInteraction");
        if (hingeInteractionClass) {
            dispatch_async(dispatch_get_main_queue(), ^{
                UIWindow *win = get_key_window();
                if (win && !g_hinge_interaction) {
                    UIHingeInteraction *interaction = [[UIHingeInteraction alloc] initWithUpdateHandler:^(UIHingeInteraction *interaction, UIHingeInteractionUpdate *update) {
                        const char *open_env = getenv("RA2_FORCE_DUO_OPEN");
                        if (open_env && (!strcmp(open_env, "1") || !strcasecmp(open_env, "yes") || !strcasecmp(open_env, "true"))) {
                            return;
                        }
                        UIHinge *hinge = update.hinge;
                        if (hinge) {
                            int closed = (hinge.status == UIHingeStatusClosed);
                            if (closed != g_is_duo_closed) {
                                g_is_duo_closed = closed;
                                fprintf(stderr, "[ios_display] Hinge status changed: %ld (is_closed=%d)\n", (long)hinge.status, g_is_duo_closed);
                                update_notice_visibility();
                            }
                        } else {
                            if (g_is_duo_closed) {
                                g_is_duo_closed = 0;
                                update_notice_visibility();
                            }
                        }
                    }];
                    g_hinge_interaction = interaction;
                    [win addInteraction:interaction];
                    fprintf(stderr, "[ios_display] UIHingeInteraction attached to window\n");
                }
            });
        }
    }
}

int ios_display_is_duo_closed(void) {
    return g_is_duo_closed;
}

void ios_display_poll(void) {
    const char *open_env = getenv("RA2_FORCE_DUO_OPEN");
    if (open_env && (!strcmp(open_env, "1") || !strcasecmp(open_env, "yes") || !strcasecmp(open_env, "true"))) {
        if (g_is_duo_closed) {
            g_is_duo_closed = 0;
            update_notice_visibility();
        }
        return;
    }
    const char *sim_env = getenv("RA2_SIMULATE_DUO_CLOSED");
    if (sim_env && (!strcmp(sim_env, "1") || !strcasecmp(sim_env, "yes") || !strcasecmp(sim_env, "true"))) {
        if (!g_is_duo_closed || !g_notice_view || !g_notice_view.superview) {
            g_is_duo_closed = 1;
            update_notice_visibility();
        }
    } else if (sim_env && (!strcmp(sim_env, "0") || !strcasecmp(sim_env, "no") || !strcasecmp(sim_env, "false"))) {
        if (g_is_duo_closed) {
            g_is_duo_closed = 0;
            update_notice_visibility();
        }
    }
}

void ios_display_get_safe_area_pixels(int renderer_w, int renderer_h,
                                     int *top, int *bottom, int *left, int *right) {
    if (top) *top = 0;
    if (bottom) *bottom = 0;
    if (left) *left = 0;
    if (right) *right = 0;

    UIWindow *win = get_key_window();
    if (win) {
        UIEdgeInsets insets = win.safeAreaInsets;
        CGRect bounds = win.bounds;
        CGFloat sx = bounds.size.width > 0 ? (CGFloat)renderer_w / bounds.size.width : 1.0;
        CGFloat sy = bounds.size.height > 0 ? (CGFloat)renderer_h / bounds.size.height : 1.0;
        if (top) *top = (int)(insets.top * sy);
        if (bottom) *bottom = (int)(insets.bottom * sy);
        if (left) *left = (int)(insets.left * sx);
        if (right) *right = (int)(insets.right * sx);
    }
}

void ios_display_get_target_resolution(int *out_w, int *out_h) {
    UIWindow *win = get_key_window();
    CGSize bounds = win ? win.bounds.size : CGSizeMake(1024, 768);
    CGFloat maxDim = MAX(bounds.width, bounds.height);
    CGFloat minDim = MIN(bounds.width, bounds.height);
    if (minDim <= 0) minDim = 1.0;
    CGFloat ratio = maxDim / minDim;

    /* Base height selection:
     * - iPad (11", 13", Mini) & iPhone Duo interior screen: 720p base height
     * - Standard iPhones (18 Pro, 18 Pro Max, etc.): 600p base height for comfortable unit size
     */
    int base_h = 720;
    UIUserInterfaceIdiom idiom = [UIDevice currentDevice].userInterfaceIdiom;
    if (idiom == UIUserInterfaceIdiomPhone) {
        BOOL isDuo = NO;
        if (@available(iOS 27.1, *)) {
            if (NSClassFromString(@"UIHingeInteraction") != nil) {
                isDuo = YES;
            }
        }
        NSString *model = [UIDevice currentDevice].model;
        NSString *name = [UIDevice currentDevice].name;
        if ([model containsString:@"Duo"] || [name containsString:@"Duo"]) {
            isDuo = YES;
        }

        if (isDuo && ratio <= 1.75f) {
            base_h = 720;
        } else {
            base_h = 600;
        }
    } else {
        base_h = 720;
    }

    int h = base_h;
    int w = ((int)(base_h * ratio + 7.0f) / 8) * 8;

    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
}

void ios_display_save_screenshot(const char *path) {
    if (!path) return;
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *win = get_key_window();
        if (!win) {
            fprintf(stderr, "[ios_display] Cannot screenshot: no UIWindow\n");
            return;
        }
        CGFloat scale = win.windowScene ? win.windowScene.screen.scale : (win.traitCollection.displayScale > 0.0 ? win.traitCollection.displayScale : 1.0);
        UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat defaultFormat];
        format.scale = scale;
        UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:win.bounds.size format:format];
        UIImage *img = [renderer imageWithActions:^(UIGraphicsImageRendererContext * _Nonnull context) {
            [win drawViewHierarchyInRect:win.bounds afterScreenUpdates:YES];
        }];
        NSData *png = UIImagePNGRepresentation(img);
        if (png) {
            [png writeToFile:[NSString stringWithUTF8String:path] atomically:YES];
            fprintf(stderr, "[ios_display] Saved screenshot (%.0fx%.0f @%.0fx, %lu bytes) to %s\n",
                    win.bounds.size.width * scale,
                    win.bounds.size.height * scale,
                    scale, (unsigned long)png.length, path);
        }
    });
}

#ifdef __cplusplus
extern "C" {
#endif
void mach_enter(void);
void mach_leave(void);
void hle_input_key(int down, uint32_t vk, uint32_t scan);
void hle_input_text(const char *text);
void hle_input_clear_focus(void);
void hle_screen_text_input_stop(void);
#ifdef __cplusplus
}
#endif

@interface RA2TextInputField : UITextField <UITextFieldDelegate>
@end

@implementation RA2TextInputField

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.delegate = self;
        self.autocapitalizationType = UITextAutocapitalizationTypeNone;
        self.autocorrectionType = UITextAutocorrectionTypeNo;
        self.spellCheckingType = UITextSpellCheckingTypeNo;
        self.smartDashesType = UITextSmartDashesTypeNo;
        self.smartQuotesType = UITextSmartQuotesTypeNo;
        self.smartInsertDeleteType = UITextSmartInsertDeleteTypeNo;
        self.keyboardType = UIKeyboardTypeDefault;
        self.returnKeyType = UIReturnKeyDone;
        self.keyboardAppearance = UIKeyboardAppearanceDark;
        self.backgroundColor = [UIColor clearColor];
        self.textColor = [UIColor clearColor];
        self.tintColor = [UIColor clearColor];
        self.alpha = 0.01f;
        self.hidden = NO;
        self.userInteractionEnabled = YES;
        self.text = @"      ";
    }
    return self;
}

- (BOOL)canBecomeFirstResponder {
    return YES;
}

- (BOOL)hasText {
    return YES;
}

- (void)deleteBackward {
    mach_enter();
    hle_input_key(1, 0x08 /* VK_BACK */, 14);
    hle_input_text("\x08");
    hle_input_key(0, 0x08 /* VK_BACK */, 14);
    mach_leave();
    self.text = @"      ";
}

- (void)insertText:(NSString *)text {
    if (!text || text.length == 0) return;
    const char *utf8 = [text UTF8String];
    if (utf8 && utf8[0]) {
        mach_enter();
        hle_input_text(utf8);
        mach_leave();
    }
    self.text = @"      ";
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    if (string.length == 0) {
        [self deleteBackward];
        return NO;
    }
    [self insertText:string];
    return NO;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    mach_enter();
    hle_input_key(1, 0x0D /* VK_RETURN */, 28);
    hle_input_key(0, 0x0D /* VK_RETURN */, 28);
    mach_leave();
    return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
    mach_enter();
    hle_input_clear_focus();
    mach_leave();
}

@end

static RA2TextInputField *g_text_input_field = nil;

void ios_display_show_keyboard(int x, int y, int w, int h) {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *win = get_key_window();
        if (!win) {
            fprintf(stderr, "[ios_display] show_keyboard: no key window found\n");
            return;
        }

        int target_x = x > 0 ? x : 50;
        int target_y = y > 0 ? y : 50;
        int target_w = w > 20 ? w : 120;
        int target_h = h > 20 ? h : 32;

        if (!g_text_input_field) {
            g_text_input_field = [[RA2TextInputField alloc] initWithFrame:CGRectMake(target_x, target_y, target_w, target_h)];
            [win addSubview:g_text_input_field];
        } else {
            g_text_input_field.frame = CGRectMake(target_x, target_y, target_w, target_h);
            if (g_text_input_field.superview != win) {
                [g_text_input_field removeFromSuperview];
                [win addSubview:g_text_input_field];
            }
        }

        g_text_input_field.hidden = NO;
        g_text_input_field.alpha = 0.01f;
        g_text_input_field.userInteractionEnabled = YES;
        g_text_input_field.text = @"      ";

        [g_text_input_field reloadInputViews];
        BOOL ok = [g_text_input_field becomeFirstResponder];
        fprintf(stderr, "[ios_display] show_keyboard: frame=(%d, %d, %d, %d) becomeFirstResponder=%s\n",
                target_x, target_y, target_w, target_h, ok ? "YES" : "NO");
    });
}

void ios_display_hide_keyboard(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_text_input_field && [g_text_input_field isFirstResponder]) {
            [g_text_input_field resignFirstResponder];
        }
        UIWindow *win = get_key_window();
        if (win) {
            [win endEditing:YES];
        }
    });
}
