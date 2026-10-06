#include "viewmodels/watch_ida_vm.h"
static const watch_ida_state_t demo={"14:32","045\xC2\xB0","6.4 kn","420 m","78%","600 m","LINK --","SIM"};
const watch_ida_state_t *watch_ida_vm_state(void) { return &demo; }
