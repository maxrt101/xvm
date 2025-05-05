#ifndef _XVM_LINKER_H_
#define _XVM_LINKER_H_ 1

#include <xvm/executable.h>
#include <vector>

namespace xvm {

Executable link(const std::vector<Executable>& objects);

} /* namespace xvm */

#endif /* _XVM_LINKER_H_ */