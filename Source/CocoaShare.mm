#import <Cocoa/Cocoa.h>

extern "C" void ShareRecipeText(const char* text,void* nativeView);
extern "C" void ShareRecipeImage(const char* imagePath,void* nativeView);
extern "C" void ActivateFridgeToTable();

extern "C" void ActivateFridgeToTable() {
    dispatch_async(dispatch_get_main_queue(), ^{
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];
    });
}

extern "C" void ShareRecipeText(const char* text,void* nativeView) {
    NSString* content = [NSString stringWithUTF8String:(text == nullptr ? "" : text)];
    NSView* suppliedView=(NSView*)nativeView;
    dispatch_async(dispatch_get_main_queue(), ^{
        NSWindow* window = NSApp.keyWindow != nil ? NSApp.keyWindow : NSApp.mainWindow;
        NSView* contentView = suppliedView != nil ? suppliedView : window.contentView;
        if (contentView == nil) return;
        [NSApp activateIgnoringOtherApps:YES];
        [contentView.window makeKeyAndOrderFront:nil];
        NSSharingServicePicker* picker = [[NSSharingServicePicker alloc] initWithItems:@[content]];
        NSRect bounds=contentView.bounds;
        NSRect anchor=NSMakeRect(NSMidX(bounds),contentView.isFlipped?44:NSMaxY(bounds)-44,1,1);
        [picker showRelativeToRect:anchor ofView:contentView preferredEdge:NSRectEdgeMaxY];
    });
}


extern "C" void ShareRecipeWithImage(const char* text,const char* imagePath);
extern "C" void ShareRecipeWithImage(const char* text,const char* imagePath) {
    NSString* content=[NSString stringWithUTF8String:text?text:""];
    NSString* path=[NSString stringWithUTF8String:imagePath?imagePath:""];
    dispatch_async(dispatch_get_main_queue(), ^{
        NSWindow* window=NSApp.keyWindow != nil ? NSApp.keyWindow : NSApp.mainWindow; NSView* anchor=window.contentView; if(!anchor)return;
        NSMutableArray* items=[NSMutableArray arrayWithObject:content];
        if(path.length>0 && [[NSFileManager defaultManager] fileExistsAtPath:path]) [items addObject:[NSURL fileURLWithPath:path]];
        NSSharingServicePicker* picker=[[NSSharingServicePicker alloc] initWithItems:items];
        [picker showRelativeToRect:anchor.bounds ofView:anchor preferredEdge:NSRectEdgeMaxY];
    });
}

// Image-only sharing deliberately excludes the text, so it can be sent as a
// separate message after the plain-text recipe.
extern "C" void ShareRecipeImage(const char* imagePath,void* nativeView) {
    NSString* path=[NSString stringWithUTF8String:imagePath?imagePath:""];
    NSView* suppliedView=(NSView*)nativeView;
    dispatch_async(dispatch_get_main_queue(), ^{
        NSWindow* window=NSApp.keyWindow != nil ? NSApp.keyWindow : NSApp.mainWindow;
        NSView* anchor=suppliedView != nil ? suppliedView : window.contentView;
        if(!anchor || path.length==0 || ![[NSFileManager defaultManager] fileExistsAtPath:path])return;
        [NSApp activateIgnoringOtherApps:YES];
        [anchor.window makeKeyAndOrderFront:nil];
        NSSharingServicePicker* picker=[[NSSharingServicePicker alloc] initWithItems:@[[NSURL fileURLWithPath:path]]];
        NSRect bounds=anchor.bounds;
        NSRect rect=NSMakeRect(NSMidX(bounds),anchor.isFlipped?44:NSMaxY(bounds)-44,1,1);
        [picker showRelativeToRect:rect ofView:anchor preferredEdge:NSRectEdgeMaxY];
    });
}
