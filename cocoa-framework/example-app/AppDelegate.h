@interface AppDelegate : NSObject <NSApplicationDelegate> {
  IBOutlet NSTextView *inputTextView_;
}

- (IBAction)parse:(id)sender;

@end
