#ifndef _XVM_LOADER_H_
#define _XVM_LOADER_H_ 1

#include <xvm/executable.h>
#include <xvm/vm.h>
#include <vector>

namespace xvm {

int load(VM& vm, Executable& exe);

} /* namespace xvm */

#endif /* _XVM_LINKER_H_ */