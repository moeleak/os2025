 #include "testkit.h"
 
 UnitTest(put_me_anywhere) {
   tk_assert(114514 == 0x114514, "This will not do.");
 }
 
 int main() {}  // cc test.c testkit.c && TK_VERBOSE= ./a.out
