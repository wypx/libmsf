
struct msf_event;
typedef struct min_heap
{
    struct msf_event** p;
    u32 n, a;
} min_heap_t;

static inline void min_heap_ctor_(min_heap_t* s);
static inline void min_heap_dtor_(min_heap_t* s);
static inline void min_heap_elem_init_(struct msf_event* e);
static inline s32 min_heap_elt_is_top_(const struct msf_event *e);
static inline s32 min_heap_empty_(min_heap_t* s);
static inline u32 min_heap_size_(min_heap_t* s);
static inline struct msf_event*  min_heap_top_(min_heap_t* s);
static inline s32 min_heap_reserve_(min_heap_t* s, u32 n);
static inline s32 min_heap_push_(min_heap_t* s, struct msf_event* e);
static inline struct msf_event*  min_heap_pop_(min_heap_t* s);
static inline s32 min_heap_adjust_(min_heap_t *s, struct msf_event* e);
static inline s32 min_heap_erase_(min_heap_t* s, struct msf_event* e);
static inline void min_heap_shift_up_(min_heap_t* s, u32 hole_index, struct msf_event* e);
static inline void min_heap_shift_up_unconditional_(min_heap_t* s, u32 hole_index, struct msf_event* e);
static inline void min_heap_shift_down_(min_heap_t* s, u32 hole_index, struct msf_event* e);

#define min_heap_elem_greater(a, b) \
    (MSF_TIMERCMP(&(a)->ev_timeout, &(b)->ev_timeout, >))

void min_heap_ctor_(min_heap_t* s) { s->p = 0; s->n = 0; s->a = 0; }
void min_heap_dtor_(min_heap_t* s) { if (s->p) sfree(s->p); }
void min_heap_elem_init_(struct msf_event* e) { e->ev_timeout_pos.min_heap_idx = -1; }
s32 min_heap_empty_(min_heap_t* s) { return 0u == s->n; }
u32 min_heap_size_(min_heap_t* s) { return s->n; }
struct msf_event* min_heap_top_(min_heap_t* s) { return s->n ? *s->p : 0; }

s32 min_heap_push_(min_heap_t* s, struct msf_event* e) {
    if (min_heap_reserve_(s, s->n + 1))
        return -1;
    min_heap_shift_up_(s, s->n++, e);
    return 0;
}

struct msf_event* min_heap_pop_(min_heap_t* s) {
    if (s->n) {
    struct msf_event* e = *s->p;
    min_heap_shift_down_(s, 0u, s->p[--s->n]);
    e->ev_timeout_pos.min_heap_idx = -1;
    return e;
    }
    return 0;
}

s32 min_heap_elt_is_top_(const struct msf_event *e) {
    return e->ev_timeout_pos.min_heap_idx == 0;
}

s32 min_heap_erase_(min_heap_t* s, struct msf_event* e) {

    if (-1 != e->ev_timeout_pos.min_heap_idx) {
    struct msf_event *last = s->p[--s->n];
    u32 parent = (e->ev_timeout_pos.min_heap_idx - 1) / 2;
    /* we replace e with the last element in the heap.  We might need to
       shift it upward if it is less than its parent, or downward if it is
       greater than one or both its children. Since the children are known
       to be less than the parent, it can't need to shift both up and
       down. */
    if (e->ev_timeout_pos.min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], last))
        min_heap_shift_up_unconditional_(s, e->ev_timeout_pos.min_heap_idx, last);
    else
        min_heap_shift_down_(s, e->ev_timeout_pos.min_heap_idx, last);
    e->ev_timeout_pos.min_heap_idx = -1;
    return 0;
    }
    return -1;
}

s32 min_heap_adjust_(min_heap_t *s, struct msf_event *e) {

    if (-1 == e->ev_timeout_pos.min_heap_idx) {
     return min_heap_push_(s, e);
    } else {
        u32 parent = (e->ev_timeout_pos.min_heap_idx - 1) / 2;
        /* The position of e has changed; we shift it up or down
         * as needed.  We can't need to do both. */
        if (e->ev_timeout_pos.min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], e))
        min_heap_shift_up_unconditional_(s, e->ev_timeout_pos.min_heap_idx, e);
        else
        min_heap_shift_down_(s, e->ev_timeout_pos.min_heap_idx, e);
        return 0;
    }
}

s32 min_heap_reserve_(min_heap_t* s, u32 n) {

    if (s->a < n) {
    struct msf_event** p;
    u32 a = s->a ? s->a * 2 : 8;
    if (a < n)
        a = n;
    if (!(p = (struct msf_event**)realloc(s->p, a * sizeof *p)))
        return -1;
    s->p = p;
    s->a = a;
    }
    return 0;
}

void min_heap_shift_up_unconditional_(min_heap_t* s, u32 hole_index, struct msf_event* e) {

    u32 parent = (hole_index - 1) / 2;
    do {
    (s->p[hole_index] = s->p[parent])->ev_timeout_pos.min_heap_idx = hole_index;
    hole_index = parent;
    parent = (hole_index - 1) / 2;
    } while (hole_index && min_heap_elem_greater(s->p[parent], e));

    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

void min_heap_shift_up_(min_heap_t* s, u32 hole_index, struct msf_event* e)
{
    unsigned parent = (hole_index - 1) / 2;
    while (hole_index && min_heap_elem_greater(s->p[parent], e)) {
        (s->p[hole_index] = s->p[parent])->ev_timeout_pos.min_heap_idx = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    }
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

void min_heap_shift_down_(min_heap_t* s, u32 hole_index, struct msf_event* e) {

    u32 min_child = 2 * (hole_index + 1);
    while (min_child <= s->n) {		
        min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);
        if (!(min_heap_elem_greater(e, s->p[min_child])))
            break;
        (s->p[hole_index] = s->p[min_child])->ev_timeout_pos.min_heap_idx = hole_index;
        hole_index = min_child;
        min_child = 2 * (hole_index + 1);
    }

    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

