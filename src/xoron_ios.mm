/*
 * xoron_ios.mm - Native iOS UI for Xoron Executor
 * Uses UIKit for native rendering on iOS (iPhone)
 * Purple & Black theme matching the executor design
 * 
 * This replaces the Lua-based Drawing UI with native UIKit components
 * for better performance and native iOS look/feel.
 */

#import <UIKit/UIKit.h>
#import <WebKit/WKWebView.h>
#import <CoreGraphics/CoreGraphics.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>

#include "xoron.h"
#include "lua.h"
#include "lualib.h"

// ============================================================================
// Theme Colors - Purple & Black
// ============================================================================
@interface XoronTheme : NSObject
+ (UIColor *)background;
+ (UIColor *)backgroundDark;
+ (UIColor *)header;
+ (UIColor *)border;
+ (UIColor *)purple;
+ (UIColor *)purpleLight;
+ (UIColor *)purpleDark;
+ (UIColor *)text;
+ (UIColor *)textDim;
+ (UIColor *)textMuted;
+ (UIColor *)green;
+ (UIColor *)red;
+ (UIColor *)yellow;
+ (UIColor *)blue;
+ (UIColor *)buttonBg;
@end

@implementation XoronTheme
+ (UIColor *)background { return [UIColor colorWithRed:12/255.0 green:12/255.0 blue:15/255.0 alpha:1.0]; }
+ (UIColor *)backgroundDark { return [UIColor colorWithRed:15/255.0 green:15/255.0 blue:18/255.0 alpha:1.0]; }
+ (UIColor *)header { return [UIColor colorWithRed:24/255.0 green:24/255.0 blue:27/255.0 alpha:1.0]; }
+ (UIColor *)border { return [UIColor colorWithRed:42/255.0 green:42/255.0 blue:58/255.0 alpha:1.0]; }
+ (UIColor *)purple { return [UIColor colorWithRed:147/255.0 green:51/255.0 blue:234/255.0 alpha:1.0]; }
+ (UIColor *)purpleLight { return [UIColor colorWithRed:168/255.0 green:85/255.0 blue:247/255.0 alpha:1.0]; }
+ (UIColor *)purpleDark { return [UIColor colorWithRed:109/255.0 green:40/255.0 blue:217/255.0 alpha:1.0]; }
+ (UIColor *)text { return [UIColor colorWithRed:228/255.0 green:228/255.0 blue:231/255.0 alpha:1.0]; }
+ (UIColor *)textDim { return [UIColor colorWithRed:113/255.0 green:113/255.0 blue:122/255.0 alpha:1.0]; }
+ (UIColor *)textMuted { return [UIColor colorWithRed:82/255.0 green:82/255.0 blue:91/255.0 alpha:1.0]; }
+ (UIColor *)green { return [UIColor colorWithRed:34/255.0 green:197/255.0 blue:94/255.0 alpha:1.0]; }
+ (UIColor *)red { return [UIColor colorWithRed:239/255.0 green:68/255.0 blue:68/255.0 alpha:1.0]; }
+ (UIColor *)yellow { return [UIColor colorWithRed:251/255.0 green:191/255.0 blue:36/255.0 alpha:1.0]; }
+ (UIColor *)blue { return [UIColor colorWithRed:96/255.0 green:165/255.0 blue:250/255.0 alpha:1.0]; }
+ (UIColor *)buttonBg { return [UIColor colorWithRed:39/255.0 green:39/255.0 blue:42/255.0 alpha:1.0]; }
@end

// ============================================================================
// Toggle Button - Floating button to open/close executor
// ============================================================================
@interface XoronToggleButton : UIView
@property (nonatomic, copy) void (^onTap)(void);
@property (nonatomic, assign) BOOL isOpen;
@end

@implementation XoronToggleButton

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.backgroundColor = [XoronTheme header];
        self.layer.cornerRadius = frame.size.width / 2;
        self.layer.borderWidth = 2;
        self.layer.borderColor = [XoronTheme purple].CGColor;
        self.clipsToBounds = YES;
        self.isOpen = NO;
        
        // Add inner circle
        UIView *innerCircle = [[UIView alloc] initWithFrame:CGRectInset(self.bounds, 6, 6)];
        innerCircle.backgroundColor = [XoronTheme purple];
        innerCircle.layer.cornerRadius = innerCircle.frame.size.width / 2;
        innerCircle.tag = 100;
        [self addSubview:innerCircle];
        
        // Add X icon
        [self addXIcon];
        
        // Add tap gesture
        UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)];
        [self addGestureRecognizer:tap];
        
        // Add pan gesture for dragging
        UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
        [self addGestureRecognizer:pan];
    }
    return self;
}

- (void)addXIcon {
    CGFloat size = 14;
    CGFloat centerX = self.bounds.size.width / 2;
    CGFloat centerY = self.bounds.size.height / 2;
    
    UIView *line1 = [[UIView alloc] initWithFrame:CGRectMake(centerX - size/2, centerY - 1, size, 2.5)];
    line1.backgroundColor = [XoronTheme text];
    line1.transform = CGAffineTransformMakeRotation(M_PI / 4);
    line1.tag = 101;
    [self addSubview:line1];
    
    UIView *line2 = [[UIView alloc] initWithFrame:CGRectMake(centerX - size/2, centerY - 1, size, 2.5)];
    line2.backgroundColor = [XoronTheme text];
    line2.transform = CGAffineTransformMakeRotation(-M_PI / 4);
    line2.tag = 102;
    [self addSubview:line2];
}

- (void)handleTap:(UITapGestureRecognizer *)gesture {
    // Haptic feedback
    UIImpactFeedbackGenerator *feedback = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
    [feedback impactOccurred];
    
    if (self.onTap) {
        self.onTap();
    }
}

- (void)handlePan:(UIPanGestureRecognizer *)gesture {
    CGPoint translation = [gesture translationInView:self.superview];
    self.center = CGPointMake(self.center.x + translation.x, self.center.y + translation.y);
    [gesture setTranslation:CGPointZero inView:self.superview];
    
    // Keep within screen bounds
    CGRect bounds = self.superview.bounds;
    CGFloat halfWidth = self.frame.size.width / 2;
    CGFloat halfHeight = self.frame.size.height / 2;
    
    if (self.center.x < halfWidth) self.center = CGPointMake(halfWidth, self.center.y);
    if (self.center.x > bounds.size.width - halfWidth) self.center = CGPointMake(bounds.size.width - halfWidth, self.center.y);
    if (self.center.y < halfHeight) self.center = CGPointMake(self.center.x, halfHeight);
    if (self.center.y > bounds.size.height - halfHeight) self.center = CGPointMake(self.center.x, bounds.size.height - halfHeight);
}

@end

// ============================================================================
// Console Message Types
// ============================================================================
typedef NS_ENUM(NSInteger, XoronMessageType) {
    XoronMessageTypeInfo = 0,
    XoronMessageTypeSuccess = 1,
    XoronMessageTypeWarning = 2,
    XoronMessageTypeError = 3,
    XoronMessageTypePrint = 4
};

// ============================================================================
// Main Executor View Controller
// ============================================================================
@interface XoronExecutorViewController : UIViewController <UITextViewDelegate, UITableViewDelegate, UITableViewDataSource>

@property (nonatomic, strong) UIView *containerView;
@property (nonatomic, strong) UIView *headerView;
@property (nonatomic, strong) UISegmentedControl *tabControl;
@property (nonatomic, strong) UITextView *editorTextView;
@property (nonatomic, strong) UITableView *consoleTableView;
@property (nonatomic, strong) UITableView *scriptsTableView;
@property (nonatomic, strong) UIView *editorContainer;
@property (nonatomic, strong) UIView *consoleContainer;
@property (nonatomic, strong) UIView *scriptsContainer;
@property (nonatomic, strong) UIView *buttonBar;
@property (nonatomic, strong) NSMutableArray<NSDictionary *> *consoleMessages;
@property (nonatomic, strong) NSMutableArray<NSDictionary *> *savedScripts;
@property (nonatomic, strong) UILabel *fpsLabel;
@property (nonatomic, strong) UILabel *pingLabel;
@property (nonatomic, strong) UIView *statusDot;

@end

@implementation XoronExecutorViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.consoleMessages = [NSMutableArray array];
    self.savedScripts = [NSMutableArray arrayWithArray:@[
        @{@"name": @"Speed Hack", @"code": @"game.Players.LocalPlayer.Character.Humanoid.WalkSpeed = 100"},
        @{@"name": @"Jump Power", @"code": @"game.Players.LocalPlayer.Character.Humanoid.JumpPower = 150"},
        @{@"name": @"Infinite Jump", @"code": @"-- Infinite Jump\nlocal uis = game:GetService(\"UserInputService\")\nuis.JumpRequest:Connect(function()\n    game.Players.LocalPlayer.Character.Humanoid:ChangeState(\"Jumping\")\nend)"}
    ]];
    
    [self setupUI];
    [self addConsoleMessage:@"Xoron Executor initialized" type:XoronMessageTypeSuccess];
    [self addConsoleMessage:@"Ready to execute scripts" type:XoronMessageTypeInfo];
}

- (void)setupUI {
    self.view.backgroundColor = [UIColor clearColor];
    
    // Main container with rounded corners
    CGFloat screenWidth = [UIScreen mainScreen].bounds.size.width;
    CGFloat screenHeight = [UIScreen mainScreen].bounds.size.height;
    CGFloat containerWidth = MIN(screenWidth * 0.85, 600);
    CGFloat containerHeight = MIN(screenHeight * 0.75, 380);
    
    self.containerView = [[UIView alloc] initWithFrame:CGRectMake(
        (screenWidth - containerWidth) / 2,
        (screenHeight - containerHeight) / 2,
        containerWidth,
        containerHeight
    )];
    self.containerView.backgroundColor = [XoronTheme background];
    self.containerView.layer.cornerRadius = 12;
    self.containerView.layer.borderWidth = 1;
    self.containerView.layer.borderColor = [XoronTheme border].CGColor;
    self.containerView.clipsToBounds = YES;
    [self.view addSubview:self.containerView];
    
    // Add pan gesture for dragging
    UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handleContainerPan:)];
    [self.containerView addGestureRecognizer:pan];
    
    [self setupHeader];
    [self setupTabs];
    [self setupEditorView];
    [self setupConsoleView];
    [self setupScriptsView];
    [self setupButtonBar];
    
    // Show editor by default
    [self switchToTab:0];
}

- (void)setupHeader {
    self.headerView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.containerView.frame.size.width, 44)];
    self.headerView.backgroundColor = [XoronTheme header];
    [self.containerView addSubview:self.headerView];
    
    // Purple accent line
    UIView *accentLine = [[UIView alloc] initWithFrame:CGRectMake(0, 43, self.headerView.frame.size.width, 2)];
    accentLine.backgroundColor = [XoronTheme purple];
    [self.headerView addSubview:accentLine];
    
    // Logo X
    UIView *logoX1 = [[UIView alloc] initWithFrame:CGRectMake(16, 16, 14, 2.5)];
    logoX1.backgroundColor = [XoronTheme purple];
    logoX1.transform = CGAffineTransformMakeRotation(M_PI / 4);
    [self.headerView addSubview:logoX1];
    
    UIView *logoX2 = [[UIView alloc] initWithFrame:CGRectMake(16, 16, 14, 2.5)];
    logoX2.backgroundColor = [XoronTheme purple];
    logoX2.transform = CGAffineTransformMakeRotation(-M_PI / 4);
    [self.headerView addSubview:logoX2];
    
    // Title
    UILabel *titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(38, 12, 80, 20)];
    titleLabel.text = @"XORON";
    titleLabel.textColor = [XoronTheme text];
    titleLabel.font = [UIFont boldSystemFontOfSize:16];
    [self.headerView addSubview:titleLabel];
    
    // Status dot
    self.statusDot = [[UIView alloc] initWithFrame:CGRectMake(105, 18, 8, 8)];
    self.statusDot.backgroundColor = [XoronTheme green];
    self.statusDot.layer.cornerRadius = 4;
    [self.headerView addSubview:self.statusDot];
    
    // Status text
    UILabel *statusLabel = [[UILabel alloc] initWithFrame:CGRectMake(118, 12, 80, 20)];
    statusLabel.text = @"Connected";
    statusLabel.textColor = [XoronTheme green];
    statusLabel.font = [UIFont systemFontOfSize:10];
    [self.headerView addSubview:statusLabel];
    
    // FPS label
    self.fpsLabel = [[UILabel alloc] initWithFrame:CGRectMake(self.headerView.frame.size.width - 130, 12, 50, 20)];
    self.fpsLabel.text = @"FPS: 60";
    self.fpsLabel.textColor = [XoronTheme textMuted];
    self.fpsLabel.font = [UIFont systemFontOfSize:10];
    [self.headerView addSubview:self.fpsLabel];
    
    // Ping label
    self.pingLabel = [[UILabel alloc] initWithFrame:CGRectMake(self.headerView.frame.size.width - 75, 12, 50, 20)];
    self.pingLabel.text = @"45ms";
    self.pingLabel.textColor = [XoronTheme textMuted];
    self.pingLabel.font = [UIFont systemFontOfSize:10];
    [self.headerView addSubview:self.pingLabel];
    
    // Close button
    UIButton *closeBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    closeBtn.frame = CGRectMake(self.headerView.frame.size.width - 36, 8, 28, 28);
    closeBtn.backgroundColor = [XoronTheme buttonBg];
    closeBtn.layer.cornerRadius = 4;
    [closeBtn setTitle:@"✕" forState:UIControlStateNormal];
    [closeBtn setTitleColor:[XoronTheme red] forState:UIControlStateNormal];
    closeBtn.titleLabel.font = [UIFont systemFontOfSize:14];
    [closeBtn addTarget:self action:@selector(closeExecutor) forControlEvents:UIControlEventTouchUpInside];
    [self.headerView addSubview:closeBtn];
}

- (void)setupTabs {
    self.tabControl = [[UISegmentedControl alloc] initWithItems:@[@"Editor", @"Console", @"Scripts"]];
    self.tabControl.frame = CGRectMake(12, 52, self.containerView.frame.size.width - 24, 32);
    self.tabControl.selectedSegmentIndex = 0;
    self.tabControl.backgroundColor = [XoronTheme header];
    self.tabControl.selectedSegmentTintColor = [XoronTheme purple];
    [self.tabControl setTitleTextAttributes:@{NSForegroundColorAttributeName: [XoronTheme textDim]} forState:UIControlStateNormal];
    [self.tabControl setTitleTextAttributes:@{NSForegroundColorAttributeName: [XoronTheme text]} forState:UIControlStateSelected];
    [self.tabControl addTarget:self action:@selector(tabChanged:) forControlEvents:UIControlEventValueChanged];
    [self.containerView addSubview:self.tabControl];
}

- (void)setupEditorView {
    CGFloat topOffset = 92;
    CGFloat bottomOffset = 56;
    CGFloat height = self.containerView.frame.size.height - topOffset - bottomOffset;
    
    self.editorContainer = [[UIView alloc] initWithFrame:CGRectMake(12, topOffset, self.containerView.frame.size.width - 24, height)];
    self.editorContainer.backgroundColor = [XoronTheme backgroundDark];
    self.editorContainer.layer.cornerRadius = 8;
    self.editorContainer.layer.borderWidth = 1;
    self.editorContainer.layer.borderColor = [XoronTheme border].CGColor;
    [self.containerView addSubview:self.editorContainer];
    
    // Editor header
    UIView *editorHeader = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.editorContainer.frame.size.width, 28)];
    editorHeader.backgroundColor = [XoronTheme header];
    [self.editorContainer addSubview:editorHeader];
    
    // File tab
    UIView *fileTab = [[UIView alloc] initWithFrame:CGRectMake(8, 4, 80, 20)];
    fileTab.backgroundColor = [XoronTheme buttonBg];
    fileTab.layer.cornerRadius = 4;
    [editorHeader addSubview:fileTab];
    
    UILabel *fileLabel = [[UILabel alloc] initWithFrame:CGRectMake(8, 2, 64, 16)];
    fileLabel.text = @"script.lua";
    fileLabel.textColor = [XoronTheme textMuted];
    fileLabel.font = [UIFont systemFontOfSize:10];
    [fileTab addSubview:fileLabel];
    
    // Text view
    self.editorTextView = [[UITextView alloc] initWithFrame:CGRectMake(0, 28, self.editorContainer.frame.size.width, height - 28)];
    self.editorTextView.backgroundColor = [UIColor clearColor];
    self.editorTextView.textColor = [XoronTheme text];
    self.editorTextView.font = [UIFont fontWithName:@"Menlo" size:12];
    if (!self.editorTextView.font) {
        self.editorTextView.font = [UIFont monospacedSystemFontOfSize:12 weight:UIFontWeightRegular];
    }
    self.editorTextView.text = @"-- Welcome to Xoron Executor!\n\nlocal player = game.Players.LocalPlayer\nlocal char = player.Character\n\nif char then\n    char.Humanoid.WalkSpeed = 100\nend\n\nprint(\"Speed boosted!\")";
    self.editorTextView.autocapitalizationType = UITextAutocapitalizationTypeNone;
    self.editorTextView.autocorrectionType = UITextAutocorrectionTypeNo;
    self.editorTextView.keyboardAppearance = UIKeyboardAppearanceDark;
    self.editorTextView.delegate = self;
    [self.editorContainer addSubview:self.editorTextView];
}

- (void)setupConsoleView {
    CGFloat topOffset = 92;
    CGFloat bottomOffset = 56;
    CGFloat height = self.containerView.frame.size.height - topOffset - bottomOffset;
    
    self.consoleContainer = [[UIView alloc] initWithFrame:CGRectMake(12, topOffset, self.containerView.frame.size.width - 24, height)];
    self.consoleContainer.backgroundColor = [XoronTheme backgroundDark];
    self.consoleContainer.layer.cornerRadius = 8;
    self.consoleContainer.layer.borderWidth = 1;
    self.consoleContainer.layer.borderColor = [XoronTheme border].CGColor;
    self.consoleContainer.hidden = YES;
    [self.containerView addSubview:self.consoleContainer];
    
    self.consoleTableView = [[UITableView alloc] initWithFrame:CGRectMake(0, 0, self.consoleContainer.frame.size.width, height) style:UITableViewStylePlain];
    self.consoleTableView.backgroundColor = [UIColor clearColor];
    self.consoleTableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    self.consoleTableView.delegate = self;
    self.consoleTableView.dataSource = self;
    [self.consoleTableView registerClass:[UITableViewCell class] forCellReuseIdentifier:@"ConsoleCell"];
    [self.consoleContainer addSubview:self.consoleTableView];
}

- (void)setupScriptsView {
    CGFloat topOffset = 92;
    CGFloat bottomOffset = 56;
    CGFloat height = self.containerView.frame.size.height - topOffset - bottomOffset;
    
    self.scriptsContainer = [[UIView alloc] initWithFrame:CGRectMake(12, topOffset, self.containerView.frame.size.width - 24, height)];
    self.scriptsContainer.backgroundColor = [XoronTheme backgroundDark];
    self.scriptsContainer.layer.cornerRadius = 8;
    self.scriptsContainer.layer.borderWidth = 1;
    self.scriptsContainer.layer.borderColor = [XoronTheme border].CGColor;
    self.scriptsContainer.hidden = YES;
    [self.containerView addSubview:self.scriptsContainer];
    
    self.scriptsTableView = [[UITableView alloc] initWithFrame:CGRectMake(0, 0, self.scriptsContainer.frame.size.width, height) style:UITableViewStylePlain];
    self.scriptsTableView.backgroundColor = [UIColor clearColor];
    self.scriptsTableView.separatorColor = [XoronTheme border];
    self.scriptsTableView.delegate = self;
    self.scriptsTableView.dataSource = self;
    [self.scriptsTableView registerClass:[UITableViewCell class] forCellReuseIdentifier:@"ScriptCell"];
    [self.scriptsContainer addSubview:self.scriptsTableView];
}

- (void)setupButtonBar {
    CGFloat buttonBarY = self.containerView.frame.size.height - 48;
    self.buttonBar = [[UIView alloc] initWithFrame:CGRectMake(12, buttonBarY, self.containerView.frame.size.width - 24, 40)];
    [self.containerView addSubview:self.buttonBar];
    
    CGFloat buttonWidth = (self.buttonBar.frame.size.width - 30) / 4;
    CGFloat buttonHeight = 36;
    
    NSArray *buttonTitles = @[@"Execute", @"Clear", @"Save", @"Copy"];
    NSArray *buttonColors = @[[XoronTheme purple], [XoronTheme buttonBg], [XoronTheme buttonBg], [XoronTheme buttonBg]];
    SEL selectors[] = {@selector(executeScript), @selector(clearEditor), @selector(saveScript), @selector(copyScript)};
    
    for (int i = 0; i < 4; i++) {
        UIButton *btn = [UIButton buttonWithType:UIButtonTypeCustom];
        btn.frame = CGRectMake(i * (buttonWidth + 10), 2, buttonWidth, buttonHeight);
        btn.backgroundColor = buttonColors[i];
        btn.layer.cornerRadius = 6;
        [btn setTitle:buttonTitles[i] forState:UIControlStateNormal];
        [btn setTitleColor:[XoronTheme text] forState:UIControlStateNormal];
        btn.titleLabel.font = [UIFont boldSystemFontOfSize:13];
        [btn addTarget:self action:selectors[i] forControlEvents:UIControlEventTouchUpInside];
        [self.buttonBar addSubview:btn];
    }
}

// ============================================================================
// Tab Switching
// ============================================================================
- (void)tabChanged:(UISegmentedControl *)sender {
    [self switchToTab:sender.selectedSegmentIndex];
}

- (void)switchToTab:(NSInteger)index {
    self.editorContainer.hidden = (index != 0);
    self.consoleContainer.hidden = (index != 1);
    self.scriptsContainer.hidden = (index != 2);
    
    // Update button bar visibility based on tab
    self.buttonBar.hidden = (index == 1); // Hide buttons on console tab
}

// ============================================================================
// Actions
// ============================================================================
- (void)executeScript {
    UIImpactFeedbackGenerator *feedback = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
    [feedback impactOccurred];
    
    NSString *code = self.editorTextView.text;
    if (code.length == 0) {
        [self addConsoleMessage:@"No script to execute" type:XoronMessageTypeWarning];
        return;
    }
    
    [self addConsoleMessage:@"Executing script..." type:XoronMessageTypeInfo];
    
    // Call the Lua execution function
    lua_State* L = xoron_get_lua_state();
    if (L) {
        lua_getglobal(L, "xoron_execute");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, [code UTF8String]);
            if (lua_pcall(L, 1, 0, 0) == 0) {
                [self addConsoleMessage:@"Script executed successfully" type:XoronMessageTypeSuccess];
            } else {
                NSString *error = [NSString stringWithUTF8String:lua_tostring(L, -1)];
                [self addConsoleMessage:[NSString stringWithFormat:@"Error: %@", error] type:XoronMessageTypeError];
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
            [self addConsoleMessage:@"Execution function not available" type:XoronMessageTypeError];
        }
    } else {
        [self addConsoleMessage:@"Lua state not initialized" type:XoronMessageTypeError];
    }
}

- (void)clearEditor {
    UIImpactFeedbackGenerator *feedback = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
    [feedback impactOccurred];
    
    self.editorTextView.text = @"";
    [self addConsoleMessage:@"Editor cleared" type:XoronMessageTypeInfo];
}

- (void)saveScript {
    UIImpactFeedbackGenerator *feedback = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
    [feedback impactOccurred];
    
    NSString *code = self.editorTextView.text;
    if (code.length == 0) {
        [self addConsoleMessage:@"Nothing to save" type:XoronMessageTypeWarning];
        return;
    }
    
    NSString *name = [NSString stringWithFormat:@"Script_%ld", (long)[[NSDate date] timeIntervalSince1970]];
    [self.savedScripts addObject:@{@"name": name, @"code": code}];
    [self.scriptsTableView reloadData];
    [self addConsoleMessage:[NSString stringWithFormat:@"Saved as %@", name] type:XoronMessageTypeSuccess];
}

- (void)copyScript {
    UIImpactFeedbackGenerator *feedback = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
    [feedback impactOccurred];
    
    NSString *code = self.editorTextView.text;
    if (code.length == 0) {
        [self addConsoleMessage:@"Nothing to copy" type:XoronMessageTypeWarning];
        return;
    }
    
    [UIPasteboard generalPasteboard].string = code;
    [self addConsoleMessage:@"Copied to clipboard" type:XoronMessageTypeInfo];
}

- (void)closeExecutor {
    UIImpactFeedbackGenerator *feedback = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
    [feedback impactOccurred];
    
    // Notify that UI should be hidden
    [[NSNotificationCenter defaultCenter] postNotificationName:@"XoronHideUI" object:nil];
}

// ============================================================================
// Console
// ============================================================================
- (void)addConsoleMessage:(NSString *)message type:(XoronMessageType)type {
    UIColor *color;
    switch (type) {
        case XoronMessageTypeSuccess: color = [XoronTheme green]; break;
        case XoronMessageTypeWarning: color = [XoronTheme yellow]; break;
        case XoronMessageTypeError: color = [XoronTheme red]; break;
        case XoronMessageTypeInfo: color = [XoronTheme blue]; break;
        default: color = [XoronTheme text]; break;
    }
    
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    formatter.dateFormat = @"HH:mm:ss";
    NSString *timestamp = [formatter stringFromDate:[NSDate date]];
    
    [self.consoleMessages addObject:@{
        @"message": message,
        @"color": color,
        @"timestamp": timestamp
    }];
    
    // Keep only last 100 messages
    while (self.consoleMessages.count > 100) {
        [self.consoleMessages removeObjectAtIndex:0];
    }
    
    [self.consoleTableView reloadData];
    
    // Scroll to bottom
    if (self.consoleMessages.count > 0) {
        NSIndexPath *lastRow = [NSIndexPath indexPathForRow:self.consoleMessages.count - 1 inSection:0];
        [self.consoleTableView scrollToRowAtIndexPath:lastRow atScrollPosition:UITableViewScrollPositionBottom animated:YES];
    }
}

// ============================================================================
// Table View Delegate/DataSource
// ============================================================================
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (tableView == self.consoleTableView) {
        return self.consoleMessages.count;
    } else {
        return self.savedScripts.count;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (tableView == self.consoleTableView) {
        UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ConsoleCell" forIndexPath:indexPath];
        cell.backgroundColor = [UIColor clearColor];
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        
        NSDictionary *msg = self.consoleMessages[indexPath.row];
        cell.textLabel.text = [NSString stringWithFormat:@"[%@] %@", msg[@"timestamp"], msg[@"message"]];
        cell.textLabel.textColor = msg[@"color"];
        cell.textLabel.font = [UIFont fontWithName:@"Menlo" size:11];
        if (!cell.textLabel.font) {
            cell.textLabel.font = [UIFont monospacedSystemFontOfSize:11 weight:UIFontWeightRegular];
        }
        cell.textLabel.numberOfLines = 0;
        
        return cell;
    } else {
        UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ScriptCell" forIndexPath:indexPath];
        cell.backgroundColor = [UIColor clearColor];
        cell.selectionStyle = UITableViewCellSelectionStyleDefault;
        
        NSDictionary *script = self.savedScripts[indexPath.row];
        cell.textLabel.text = script[@"name"];
        cell.textLabel.textColor = [XoronTheme text];
        cell.textLabel.font = [UIFont systemFontOfSize:14];
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        
        return cell;
    }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    if (tableView == self.scriptsTableView) {
        NSDictionary *script = self.savedScripts[indexPath.row];
        self.editorTextView.text = script[@"code"];
        self.tabControl.selectedSegmentIndex = 0;
        [self switchToTab:0];
        [self addConsoleMessage:[NSString stringWithFormat:@"Loaded %@", script[@"name"]] type:XoronMessageTypeInfo];
    }
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 32;
}

// ============================================================================
// Dragging
// ============================================================================
- (void)handleContainerPan:(UIPanGestureRecognizer *)gesture {
    CGPoint translation = [gesture translationInView:self.view];
    self.containerView.center = CGPointMake(self.containerView.center.x + translation.x, self.containerView.center.y + translation.y);
    [gesture setTranslation:CGPointZero inView:self.view];
}

@end

// ============================================================================
// Global UI Manager
// ============================================================================
static XoronToggleButton *g_toggleButton = nil;
static XoronExecutorViewController *g_executorVC = nil;
static UIWindow *g_overlayWindow = nil;
static lua_State *g_lua_state = nil;

// Get Lua state
extern "C" lua_State* xoron_get_lua_state(void) {
    return g_lua_state;
}

// Set Lua state
extern "C" void xoron_ios_set_lua_state(lua_State* L) {
    g_lua_state = L;
}

// Initialize the iOS UI
extern "C" void xoron_ios_ui_init(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_overlayWindow) return; // Already initialized
        
        // Create overlay window
        g_overlayWindow = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
        g_overlayWindow.windowLevel = UIWindowLevelAlert + 100;
        g_overlayWindow.backgroundColor = [UIColor clearColor];
        g_overlayWindow.hidden = NO;
        g_overlayWindow.userInteractionEnabled = YES;
        
        // Create root view controller
        UIViewController *rootVC = [[UIViewController alloc] init];
        rootVC.view.backgroundColor = [UIColor clearColor];
        g_overlayWindow.rootViewController = rootVC;
        
        // Create toggle button
        CGFloat buttonSize = 52;
        CGFloat screenWidth = [UIScreen mainScreen].bounds.size.width;
        g_toggleButton = [[XoronToggleButton alloc] initWithFrame:CGRectMake(
            screenWidth - buttonSize - 20,
            20,
            buttonSize,
            buttonSize
        )];
        g_toggleButton.onTap = ^{
            xoron_ios_ui_toggle();
        };
        [rootVC.view addSubview:g_toggleButton];
        
        // Create executor view controller
        g_executorVC = [[XoronExecutorViewController alloc] init];
        g_executorVC.view.frame = rootVC.view.bounds;
        g_executorVC.view.hidden = YES;
        [rootVC addChildViewController:g_executorVC];
        [rootVC.view insertSubview:g_executorVC.view belowSubview:g_toggleButton];
        [g_executorVC didMoveToParentViewController:rootVC];
        
        // Listen for hide notification
        [[NSNotificationCenter defaultCenter] addObserverForName:@"XoronHideUI" object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
            xoron_ios_ui_hide();
        }];
        
        [g_overlayWindow makeKeyAndVisible];
    });
}

// Show the executor UI
extern "C" void xoron_ios_ui_show(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!g_executorVC) {
            xoron_ios_ui_init();
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
                g_executorVC.view.hidden = NO;
                g_executorVC.view.alpha = 0;
                [UIView animateWithDuration:0.2 animations:^{
                    g_executorVC.view.alpha = 1;
                }];
            });
        } else {
            g_executorVC.view.hidden = NO;
            g_executorVC.view.alpha = 0;
            [UIView animateWithDuration:0.2 animations:^{
                g_executorVC.view.alpha = 1;
            }];
        }
    });
}

// Hide the executor UI
extern "C" void xoron_ios_ui_hide(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_executorVC) {
            [UIView animateWithDuration:0.2 animations:^{
                g_executorVC.view.alpha = 0;
            } completion:^(BOOL finished) {
                g_executorVC.view.hidden = YES;
            }];
        }
    });
}

// Toggle the executor UI
extern "C" void xoron_ios_ui_toggle(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!g_executorVC) {
            xoron_ios_ui_init();
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
                xoron_ios_ui_show();
            });
        } else if (g_executorVC.view.hidden) {
            xoron_ios_ui_show();
        } else {
            xoron_ios_ui_hide();
        }
    });
}

// Haptic feedback
extern "C" void xoron_ios_haptic_feedback(int style) {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIImpactFeedbackStyle feedbackStyle;
        switch (style) {
            case 0: feedbackStyle = UIImpactFeedbackStyleLight; break;
            case 2: feedbackStyle = UIImpactFeedbackStyleHeavy; break;
            default: feedbackStyle = UIImpactFeedbackStyleMedium; break;
        }
        UIImpactFeedbackGenerator *feedback = [[UIImpactFeedbackGenerator alloc] initWithStyle:feedbackStyle];
        [feedback impactOccurred];
    });
}

// Add console message from C++
extern "C" void xoron_ios_console_print(const char* message, int type) {
    if (!message) return;
    
    NSString *msg = [NSString stringWithUTF8String:message];
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_executorVC) {
            [g_executorVC addConsoleMessage:msg type:(XoronMessageType)type];
        }
    });
}

// Register iOS-specific Lua functions
void xoron_register_ios(lua_State* L) {
    g_lua_state = L;
    
    // Initialize UI on main thread
    xoron_ios_ui_init();
    
    // Register Lua functions for UI control
    lua_newtable(L);
    
    lua_pushcfunction(L, [](lua_State* L) -> int {
        xoron_ios_ui_show();
        return 0;
    }, "show");
    lua_setfield(L, -2, "show");
    
    lua_pushcfunction(L, [](lua_State* L) -> int {
        xoron_ios_ui_hide();
        return 0;
    }, "hide");
    lua_setfield(L, -2, "hide");
    
    lua_pushcfunction(L, [](lua_State* L) -> int {
        xoron_ios_ui_toggle();
        return 0;
    }, "toggle");
    lua_setfield(L, -2, "toggle");
    
    lua_pushcfunction(L, [](lua_State* L) -> int {
        const char* msg = luaL_checkstring(L, 1);
        int type = luaL_optinteger(L, 2, 0);
        xoron_ios_console_print(msg, type);
        return 0;
    }, "print");
    lua_setfield(L, -2, "print");
    
    // iOS-specific input functions
    lua_pushcfunction(L, [](lua_State* L) -> int {
        float x = (float)luaL_checknumber(L, 1);
        float y = (float)luaL_checknumber(L, 2);
        bool press = lua_toboolean(L, 3);
        
        // Simulate touch input
        dispatch_async(dispatch_get_main_queue(), ^{
            // This would integrate with the game's input system
            // TODO: Implement actual touch simulation for the game
        });
        
        return 0;
    }, "simulateTouch");
    lua_setfield(L, -2, "simulateTouch");
    
    lua_setglobal(L, "XoronNative");
}
