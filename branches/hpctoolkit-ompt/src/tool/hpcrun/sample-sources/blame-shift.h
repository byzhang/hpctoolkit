#ifndef blame_shift_h
#define blame_shift_h

#include <cct/cct.h>

#ifdef XU_OLD
typedef void (*bs_fn_t)(cct_node_t *node, uint64_t metric_value);
#endif // XU_OLD
typedef void (*bs_fn_t)(int metric_id, cct_node_t *node, int metric_incr);

typedef struct bs_fn_entry_s {
 struct bs_fn_entry_s *next;
 bs_fn_t fn;
} bs_fn_entry_t;

typedef enum bs_type{
  bs_type_timer,
  bs_type_cycles
} bs_type;

void blame_shift_register(bs_fn_entry_t *entry);
#ifdef XU_OLD
void blame_shift_apply(cct_node_t *node, uint64_t metric_value);
#endif // XU_OLD

void blame_shift_apply(int metric_id, cct_node_t *node, int metric_incr);
void blame_shift_source_register(bs_type bst);
int blame_shift_source_available(bs_type bst);

#endif