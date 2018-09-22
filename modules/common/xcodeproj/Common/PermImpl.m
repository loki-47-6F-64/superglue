//
//  PermImpl.m
//  Common
//
//  Created by Petra Farber on 19-09-18.
//  Copyright © 2018 Werkstation 4. All rights reserved.
//

#include "PermImpl.h"
#include "uPermission.h"
#include "uPermissionCallback.h"

@implementation PermImpl

-(BOOL)has:(uPermission)permission {
    return true;
}

- (void)request:(uPermission)perm f:(nullable uPermissionCallback *)f {
    
}
@end
