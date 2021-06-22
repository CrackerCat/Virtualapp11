//
// Created by 86151 on 2021/5/16.
//

#ifndef VIRTUALAPP11_MASTER_IORELOCATOR_EXPORTS_H
#define VIRTUALAPP11_MASTER_IORELOCATOR_EXPORTS_H




#   if !defined(__LP64__)
static int new___open(const char *pathname, int flags, int mode);
#   endif/*!__LP64__*/
static int new___openat(int fd, const char *pathname, int flags, int mode);




#endif //VIRTUALAPP11_MASTER_IORELOCATOR_EXPORTS_H
