#ifndef CppBridge_h
#define CppBridge_h

#ifdef __OBJC__
#import <React/RCTBridgeModule.h>

@interface CppBridge : NSObject <RCTBridgeModule>
@end
#endif

#endif /* CppBridge_h */
