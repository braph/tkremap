#include <assert.h>
#include <termkey.h>

int main() {
   assert(TERMKEY_TYPE_UNICODE  == 0);
   assert(TERMKEY_TYPE_FUNCTION == 1);
   assert(TERMKEY_TYPE_KEYSYM   == 2);
   assert(TERMKEY_TYPE_MOUSE    == 3);
   return 0;
}
