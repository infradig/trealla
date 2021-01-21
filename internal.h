#pragma once

#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <sys/param.h>

#ifndef USE_OPENSSL
#define USE_OPENSSL 0
#endif

#ifndef USE_LIBRESSL
#define USE_LIBRESSL 0
#endif

#ifndef USE_INT128
#define USE_INT128 0
#endif

#ifndef USE_INT32
#define USE_INT32 0
#endif

#ifndef USE_GMP
#define USE_GMP 0
#endif

#ifndef USE_LDLIBS
#define USE_LDLIBS 0
#endif

#ifndef USE_THREADS
#define USE_THREADS 0
#endif

#if USE_INT128
typedef __int128_t int_t;
typedef __uint128_t uint_t;
#elif USE_INT32
typedef __int32_t int_t;
typedef __uint32_t uint_t;
#else
typedef __int64_t int_t;
typedef __uint64_t uint_t;
#endif

#if (__STDC_VERSION__ >= 201112L) && USE_THREADS
#include <stdatomic.h>
#define atomic_t _Atomic
#else
#define atomic_t volatile
#endif

typedef uint32_t idx_t;

#include "skiplist.h"
#include "trealla.h"
#include "cdebug.h"

// Sentinel Value
#define ERR_IDX (~(idx_t)0)
#define IDX_MAX (ERR_IDX-1)
#define ERR_CYCLE_CMP -2

#define MAX_SMALL_STRING (MAX(sizeof(int_t),sizeof(void*))*2)
#define MAX_VAR_POOL_SIZE 1000
#define MAX_ARITY UCHAR_MAX
#define MAX_USER_OPS 250
#define MAX_QUEUES 16
#define MAX_STREAMS 1024
#define MAX_DEPTH 9000

#define STREAM_BUFLEN 1024
#define CHECK_OVERFLOW 1

#define GET_FRAME(i) (q->frames+(i))
#define GET_SLOT(g,i) ((i) < g->nbr_slots ? (q->slots+g->ctx+(i)) : (q->slots+g->overflow+((i)-g->nbr_slots)))

// Primary type...

#define is_empty(c) ((c)->val_type == TYPE_EMPTY)
#define is_variable(c) ((c)->val_type == TYPE_VARIABLE)
#define is_literal(c) ((c)->val_type == TYPE_LITERAL)
#define is_cstring(c) ((c)->val_type == TYPE_CSTRING)
#define is_rational(c) ((c)->val_type == TYPE_INTEGER)
#define is_bignum(c) ((c)->val_type == TYPE_BIGNUM)
#define is_float(c) ((c)->val_type == TYPE_FLOAT)
#define is_indirect(c) ((c)->val_type == TYPE_INDIRECT)
#define is_end(c) ((c)->val_type == TYPE_END)

// Derived type...

#define is_iso_atom(c) ((is_literal(c) || is_cstring(c)) && !(c)->arity)
#define is_iso_list(c) (is_literal(c) && ((c)->arity == 2) && ((c)->val_off == g_dot_s))

#define is_atom(c) ((is_literal(c) && !(c)->arity) || is_cstring(c))
#define is_string(c) ((c)->flags & FLAG_STRING)
#define is_blob(c) ((c)->flags & FLAG_BLOB)
#define is_list(c) (is_iso_list(c) || is_string(c))
#define is_integer(c) (is_rational(c) && ((c)->val_den == 1))
#define is_const_cstring(c) (is_cstring(c) && ((c)->flags & FLAG2_CONST))
#define is_tmp(c) ((c)->flags & FLAG_TMP)
#define is_const_blob(c) (((c)->flags & FLAG2_CONST) && is_blob(c))
#define is_nonconst_blob(c) (is_cstring(c) && !((c)->flags & FLAG2_CONST) && is_blob(c))
#define is_dup_cstring(c) (is_cstring(c) && ((c)->flags & FLAG2_DUP))
#define is_nil(c) (is_literal(c) && !(c)->arity && ((c)->val_off == g_nil_s))
#define is_quoted(c) ((c)->flags & FLAG2_QUOTED)
#define is_fresh(c) ((c)->flags & FLAG2_FRESH)
#define is_anon(c) ((c)->flags & FLAG2_ANON)
#define is_builtin(c) ((c)->flags & FLAG_BUILTIN)
#define is_key(c) ((c)->flags & FLAG_KEY)
#define is_op(c) (c->flags && 0xFF00)

// These 2 assume literal or cstring types...

#define GET_STR(c) (!is_cstring(c) ? (q->m->pl->pool + (c)->val_off) : is_blob(c) ? (c)->val_str : (c)->val_chr)
#define LEN_STR(c) (is_blob(c) ? (c)->len_str : strlen(GET_STR(c)))

#define PARSER_GET_STR(c) (!is_cstring(c) ? (p->m->pl->pool + (c)->val_off) : is_blob(c) ? (c)->val_str : (c)->val_chr)
#define PARSER_LEN_STR(c) (is_blob(c) ? (c)->len_str : strlen(PARSER_GET_STR(c)))

#define MODULE_GET_STR(c) (!is_cstring(c) ? (m->pl->pool + (c)->val_off) : is_blob(c) ? (c)->val_str : (c)->val_chr)
#define MODULE_LEN_STR(c) (is_blob(c) ? (c)->len_str : strlen(MODULE_GET_STR(c)))

#define FREE_STR(c) if (is_nonconst_blob(c)) { free((c)->val_str); }
#define TAKE_STR(c) {(c)->val_str = NULL; }

#define DUP_STR(c,v) {												\
	(c)->val_str = malloc((v)->len_str + 1);						\
	memcpy((c)->val_str, v->val_str, v->len_str); 					\
	(c)->val_str[(v)->len_str] = '\0'; }

#define QUERY_GET_POOL(off) (q->m->pl->pool + off)
#define MODULE_GET_POOL(off) (m->pl->pool + off)
#define PARSER_GET_POOL(off) (p->m->pl->pool + off)

// Wrap an assignment that's expected to return anything but the given sentinel value.
// when the sentinel otherwise does some (optional) error handling action
// default action is 'error=true' to indicate an error happened
#define CHECK_SENTINEL(expr, err_sentinel, ...) CHECK_SENTINEL_((expr), err_sentinel, ## __VA_ARGS__, error=true)
#define CHECK_SENTINEL_(expr, err_sentinel, on_error, ...) do { if((expr) == err_sentinel){message(#expr " = " #err_sentinel); on_error;}} while (0)

#define may_error(expr, ...) CHECK_SENTINEL(expr, pl_error, __VA_ARGS__; return pl_error)
#define may_idx_error(expr, ...) CHECK_SENTINEL(expr, ERR_IDX, __VA_ARGS__; return pl_error)
#define may_ptr_error(expr, ...) CHECK_SENTINEL(expr, NULL, __VA_ARGS__; return pl_error)
#define may_cycle_error(expr, ...) CHECK_SENTINEL(expr, ERR_CYCLE_CELL, __VA_ARGS__; return pl_cycle)

// If changing the order of these: see runtime.c dispatch table

enum {
	TYPE_EMPTY=0,
	TYPE_VARIABLE,
	TYPE_LITERAL,
	TYPE_CSTRING,
	TYPE_INTEGER,
	TYPE_BIGNUM,
	TYPE_FLOAT,
	TYPE_INDIRECT,
	TYPE_END
};

enum {
	FLAG_BUILTIN=1<<0,
	FLAG_HEX=1<<1,						// used with TYPE_INTEGER
	FLAG_OCTAL=1<<2,					// used with TYPE_INTEGER
	FLAG_BINARY=1<<3,					// used with TYPE_INTEGER
	FLAG_STREAM=1<<4,					// used with TYPE_INTEGER
	FLAG_TAIL_REC=1<<5,
	FLAG_BLOB=1<<6,						// used with TYPE_CSTRING
	FLAG_STRING=1<<7,					// used with TYPE_CSTRING
	FLAG_TMP=1<<8,						// used with TYPE_CSTRING
	FLAG_KEY=1<<9,						// used with keys

	FLAG_SPARE3=1<<10,
	FLAG_SPARE2=1<<11,
	FLAG_SPARE1=1<<12,

	// These are redefinitions and should only be used
	// when the primary type is already checked...

	FLAG2_PROCESSED=FLAG_KEY,			// used by bagof
	FLAG2_FIRST_USE=FLAG_HEX,			// used with TYPE_VARIABLE
	FLAG2_ANON=FLAG_OCTAL,				// used with TYPE_VARIABLE
	FLAG2_FRESH=FLAG_BINARY,			// used with TYPE_VARIABLE
	FLAG2_CONST=FLAG_HEX,				// used with TYPE_CSTRING
	FLAG2_DUP=FLAG_OCTAL,				// used with TYPE_CSTRING
	FLAG2_QUOTED=FLAG_BINARY,			// used with TYPE_CSTRING

	FLAG_END=1<<13
};

// The OP types are stored in the high 3 bits of the flag (13-15)

#define	OP_FX 1
#define	OP_FY 2
#define	OP_XF 3
#define	OP_YF 4
#define	OP_YFX 5
#define	OP_XFX 6
#define	OP_XFY 7

#define IS_PREFIX(op) ((op == OP_FX) || (op == OP_FY))
#define IS_POSTFIX(op) ((op == OP_XF) || (op == OP_YF))
#define IS_INFIX(op) ((op == OP_XFX) || (op == OP_XFY) || (op == OP_YFX))

#define CELL_PREFIX(c) IS_PREFIX(GET_OP(c))
#define CELL_POSTFIX(c) IS_POSTFIX(GET_OP(c))
#define CELL_INFIX(c) IS_INFIX(GET_OP(c))

#define IS_FX(c) (GET_OP(c) == OP_FX)
#define IS_FY(c) (GET_OP(c) == OP_FY)
#define IS_XF(c) (GET_OP(c) == OP_XF)
#define IS_YF(c) (GET_OP(c) == OP_YF)
#define IS_YFX(c) (GET_OP(c) == OP_YFX)
#define IS_XFX(c) (GET_OP(c) == OP_XFX)
#define IS_XFY(c) (GET_OP(c) == OP_XFY)

#define SET_OP(c,op) (CLR_OP(c), (c)->flags |= (((uint16_t)(op)) << 13))
#define CLR_OP(c) ((c)->flags &= ~((uint16_t)(0xF) << 13))
#define GET_OP(c) (((c)->flags >> 13) & 0xF)
#define IS_OP(c) (GET_OP(c) != 0 ? true : false)

typedef struct module_ module;
typedef struct query_ query;
typedef struct predicate_ predicate;
typedef struct clause_ clause;
typedef struct cell_ cell;
typedef struct parser_ parser;

struct cell_ {
	uint8_t val_type;
	uint8_t arity;
	uint16_t flags;
	idx_t nbr_cells;

	// The following unions are based off 'val_type' ...

	union {
		struct {
			int_t val_num;
			int_t val_den;
		};

		struct {
			double val_flt;
		};

		struct {
			char val_chr[MAX_SMALL_STRING];
		};

		struct {
			cell *val_ptr;
		};

		struct {
			char *val_str;
			size_t len_str;
		};

		struct {
			union {
				pl_state (*fn)(query*);
				predicate *match;
				cell *attrs;
				uint16_t priority;
			};

			idx_t val_off;

			union {
				idx_t var_nbr;			// used with TYPE_VAR
				idx_t cgen;				// used with cuts
			};
		};
	};
};

extern cell* ERR_CYCLE_CELL;

typedef struct {
	uint64_t u1, u2;
} uuid;

typedef struct {
	uint64_t gen;
	idx_t nbr_cells, cidx;
	uint16_t nbr_vars;
	bool first_cut:1;
	bool cut_only:1;
	bool deleted:1;
	bool persist:1;
	cell cells[];
} term;

struct clause_ {
	predicate *parent;
	clause *next;
	module *m;
	uuid u;
	term t;
};

struct predicate_ {
	predicate *next;
	clause *head, *tail;
	skiplist *index, *index_save;
	cell key;
	uint64_t gen;
	unsigned cnt;
	bool is_prebuilt:1;
	bool is_public:1;
	bool is_dynamic:1;
	bool is_persist:1;
	bool is_multifile:1;
	bool is_discontiguous:1;
	bool is_abolished:1;
	bool is_noindex:1;
	bool check_directive:1;
};

struct builtins {
	const char *name;
	unsigned arity;
	pl_state (*fn)(query*);
	const char *help;
};

struct op_table {
	const char *name;
	unsigned specifier;
	unsigned priority;
};

typedef struct {
	idx_t ctx;
	uint16_t var_nbr;
} trail;

typedef struct {
	cell c;
	idx_t ctx;
} slot;

typedef struct {
	cell *curr_cell;
	module *m;
	idx_t prev_frame, ctx, overflow, cgen;
	uint16_t nbr_vars, nbr_slots;
	bool any_choices:1;
	bool did_cut:1;
} frame;

enum { eof_action_eof_code, eof_action_error, eof_action_reset };

typedef struct {
	FILE *fp;
	char *mode, *filename, *name, *data, *src;
	void *sslptr;
	parser *p;
	char srcbuf[STREAM_BUFLEN];
	size_t data_len, alloc_nbytes;
	int ungetch, srclen;
	uint8_t level, eof_action;
	bool at_end_of_file:1;
	bool binary:1;
	bool did_getc:1;
	bool socket:1;
	bool nodelay:1;
	bool nonblock:1;
	bool udp:1;
	bool ssl:1;
} stream;

typedef struct {
	cell *curr_cell;
	clause *curr_clause, *curr_clause2;
	sliter *iter, *iter2;
	idx_t curr_frame, fp, hp, tp, sp, cgen;
	uint8_t anbr, qnbr;
} state;

typedef struct {
	state st;
	uint64_t pins;
	idx_t v1, v2, cgen, orig_cgen, overflow;
	uint16_t nbr_vars, nbr_slots;
	bool any_choices:1;
	bool catchme1:1;
	bool catchme2:1;
} choice;

typedef struct arena_ arena;

struct arena_ {
	arena *next;
	cell *heap;
	idx_t hp, h_size;
	unsigned nbr;
};

enum q_retry { QUERY_OK=0, QUERY_RETRY=1, QUERY_EXCEPTION=2 };
enum unknowns { UNK_FAIL=0, UNK_ERROR=1, UNK_WARNING=2, UNK_CHANGEABLE=3 };

typedef struct char_flags_ {
	enum unknowns unknown;
	bool double_quote_codes:1;
	bool double_quote_chars:1;
	bool double_quote_atom:1;
	bool character_escapes:1;
	bool char_conversion:1;
	bool rational_syntax_natural:1;
	bool prefer_rationals:1;
	bool debug:1;
} char_flags;

struct query_ {
	query *prev, *next, *parent;
	module *m, *save_m;
	parser *p;
	frame *frames;
	slot *slots;
	choice *choices;
	trail *trails;
	cell *tmp_heap, *last_arg, *exception, *variable_names;
	cell *queue[MAX_QUEUES], *tmpq[MAX_QUEUES];
	arena *arenas;
	cell accum;
	state st;
	uint64_t tot_goals, tot_retries, tot_matches, tot_tcos;
	uint64_t step, qid, time_started;
	unsigned max_depth, tmo_msecs;
	int nv_start;
	idx_t cp, tmphp, latest_ctx, popp, variable_names_ctx;
	idx_t frames_size, slots_size, trails_size, choices_size;
	idx_t max_choices, max_frames, max_slots, max_trails;
	idx_t h_size, tmph_size, tot_heaps, tot_heapsize;
	idx_t q_size[MAX_QUEUES], tmpq_size[MAX_QUEUES], qp[MAX_QUEUES];
	uint8_t nv_mask[MAX_ARITY];
	char_flags flag;
	enum q_retry retry;
	int8_t halt_code;
	int8_t quoted;
	bool do_dump_vars:1;
	bool status:1;
	bool resume:1;
	bool no_tco:1;
	bool error:1;
	bool did_throw:1;
	bool trace:1;
	bool calc:1;
	bool yielded:1;
	bool is_task:1;
	bool nl:1;
	bool fullstop:1;
	bool ignore_ops:1;
	bool numbervars:1;
	bool halt:1;
	bool abort:1;
	bool cycle_error:1;
	bool spawned:1;
	bool run_init:1;
};

struct parser_ {
	struct {
		char var_pool[MAX_VAR_POOL_SIZE];
		unsigned var_used[MAX_ARITY];
		const char *var_name[MAX_ARITY];
	} vartab;

	FILE *fp;
	module *m;
	term *t;
	char *token, *save_line, *srcptr;
	size_t token_size, n_line, len_str;
	char_flags flag;
	unsigned line_nbr, depth, read_term;
	int quote_char;							// C character is an int
	unsigned nbr_vars;
	uint8_t val_type;
	int8_t dq_consing;
	bool error;
	bool already_loaded:1;
	bool do_read_term:1;
	bool string:1;
	bool run_init:1;
	bool directive:1;
	bool consulting:1;
	bool one_shot:1;
	bool start_term:1;
	bool end_of_term:1;
	bool comment:1;
	bool is_quoted:1;
	bool is_variable:1;
	bool is_op:1;
	bool skip:1;
	bool command:1;
};

struct module_ {
	module *next;
	prolog *pl;
	query *tasks;
	char *name, *filename;
	predicate *head, *tail;
	parser *p;
	FILE *fp;
	skiplist *index;
	const char *tmp_filename;
	struct op_table ops[MAX_USER_OPS+1];
	char_flags flag;
	unsigned user_ops;
	bool prebuilt:1;
	bool dirty:1;
	bool use_persist:1;
	bool loading:1;
	bool make_public:1;
	bool error:1;
};

struct prolog_ {
	idx_t tab1[64000];
	idx_t tab3[64000];
	idx_t tab2[64000];
	idx_t tab4[64000];
	uint8_t tab5[64000];
	module *modules;
	module *m, *curr_m;
	uint64_t s_last, s_cnt, seed;
	skiplist *symtab, *funtab;
	char *pool;
	idx_t pool_offset, pool_size, tab_idx;
	unsigned varno;
	uint8_t current_input, current_output, current_error;
	int8_t halt_code, opt;
	bool halt:1;
	bool status:1;
	bool did_dump_vars:1;
	bool quiet:1;
	bool stats:1;
	bool noindex:1;
	bool iso_only:1;
	bool trace:1;
};

extern idx_t g_empty_s, g_pair_s, g_dot_s, g_cut_s, g_nil_s, g_true_s, g_fail_s;
extern idx_t g_anon_s, g_clause_s, g_eof_s, g_lt_s, g_false_s;
extern idx_t g_gt_s, g_eq_s, g_sys_elapsed_s, g_sys_queue_s, g_braces_s;
extern idx_t g_stream_property_s;
extern stream g_streams[MAX_STREAMS];
extern char *g_pool;
extern unsigned g_cpu_count;

inline static idx_t copy_cells(cell *dst, const cell *src, idx_t nbr_cells)
{
	memcpy(dst, src, sizeof(cell)*(nbr_cells));
	return nbr_cells;
}

#define LIST_HANDLER(l) cell l##_h_tmp; cell l##_t_tmp
#define LIST_HEAD(l) list_head(l, &l##_h_tmp)
#define LIST_TAIL(l) list_tail(l, &l##_t_tmp)

cell *list_head(cell *l, cell *tmp);
cell *list_tail(cell *l, cell *tmp);

enum {DO_CLAUSE, DO_RETRACT, DO_RETRACTALL};

USE_RESULT size_t alloc_grow(void** addr, size_t elem_size, size_t min_elements, size_t max_elements);
pl_state set_var(query *q, cell *c, idx_t ctx, cell *v, idx_t v_ctx);
void reset_value(query *q, cell *c, idx_t c_ctx, cell *v, idx_t v_ctx);
bool module_load_fp(module *m, FILE *fp, const char *filename);
bool module_load_file(module *m, const char *filename);
bool module_save_file(module *m, const char *filename);
bool deconsult(prolog *pl, const char *filename);
module *create_module(prolog *pl, const char *name);
void destroy_module(module *m);
module *find_module(prolog *pl, const char *name);
module *find_next_module(prolog *pl, module *m);
clause *asserta_to_db(module *m, term *t, bool consulting);
clause *assertz_to_db(module *m, term *t, bool consulting);
void retract_from_db(module *m, clause *r);
clause *erase_from_db(module *m, uuid *ref);
clause *find_in_db(module *m, uuid *ref);
unsigned get_op(module *m, const char *name, unsigned *specifier, bool *userop, bool hint_prefix);
bool set_op(module *m, const char *name, unsigned specifier, unsigned priority);
USE_RESULT pl_state make_choice(query *q);
USE_RESULT pl_state make_barrier(query *q);
USE_RESULT pl_state make_catcher(query *q, enum q_retry type);
USE_RESULT pl_state fn_iso_cut_0(query *q);
void cut_me(query *q);
bool check_builtin(prolog *pl, const char *name, unsigned arity);
void *get_builtin(prolog *pl, const char *name, unsigned arity);
pl_state query_execute(query *q, term *t);
bool check_rule(const cell *c);
cell *get_head(cell *c);
cell *get_body(cell *c);
cell *get_logical_body(cell *c);
predicate *find_predicate(module *m, cell *c);
predicate *find_matching_predicate(module *m, cell *c);
predicate *find_matching_predicate_quiet(module *m, cell *c);
predicate *find_functor(module *m, const char *name, unsigned arity);
USE_RESULT pl_state fn_call_0(query *q, cell *p1);
void undo_me(query *q);
parser *create_parser(module *m);
void destroy_parser(parser *p);
void destroy_parser_nodelete(parser *p);
unsigned parser_tokenize(parser *p, bool args, bool consing);
void parser_xref(parser *p, term *t, predicate *parent);
void parser_reset(parser *p);
idx_t drop_choice(query *q);
bool retry_choice(query *q);
void parser_assign_vars(parser *p, unsigned start, bool rebase);
query *create_query(module *m, bool sub_query);
query *create_task(query *q, cell *curr_cell);
void destroy_query(query *q);
USE_RESULT pl_state run_query(query *q);
USE_RESULT cell *deep_clone_to_heap(query *q, cell *p1, idx_t p1_ctx);
USE_RESULT cell *clone_to_heap(query *q, bool prefix, cell *p1, idx_t suffix);
void make_end(cell *tmp);
USE_RESULT pl_state match_rule(query *q, cell *p1, idx_t p1_ctx);
USE_RESULT pl_state match_clause(query *q, cell *p1, idx_t p1_ctx, int retract);
idx_t index_from_pool(prolog *pl, const char *name);
void do_reduce(cell *n);
unsigned create_vars(query *q, unsigned nbr);
unsigned count_bits(const uint8_t *mask, unsigned bit);
void try_me(const query *q, unsigned vars);
USE_RESULT pl_state throw_error(query *q, cell *c, const char *err_type, const char *expected);
uint64_t get_time_in_usec(void);
void clear_term(term *t);
void do_db_load(module *m);
size_t sprint_int(char *dst, size_t size, int_t n, int base);
void call_attrs(query *q, cell *attrs);
void allocate_list_on_heap(query *q, const cell *c);
void append_list(query *q, const cell *c);
USE_RESULT cell *end_list(query *q);
size_t scan_is_chars_list(query *q, cell *l, idx_t l_ctx, int tolerant);
void consultall(parser *p, cell *l);
void fix_list(cell *c);
module *module_load_text(module *m, const char *src);
void make_indirect(cell *tmp, cell *c);
void stash_me(query *q, term *t, bool last_match);
unsigned fake_numbervars(query *q, cell *c, idx_t c_ctx, unsigned start);
char *relative_to(const char *basefile, const char *relfile);
void parser_term_to_body(parser *p);
cell *check_body_callable(parser *p, cell *c);
void load_builtins(prolog *pl, bool iso_only);

ssize_t print_term_to_buf(query *q, char *dst, size_t dstlen, cell *c, idx_t c_ctx, int running, int cons, unsigned depth);
pl_state print_term(query *q, FILE *fp, cell *c, idx_t c_ctx, int running);
pl_state print_term_to_stream(query *q, stream *str, cell *c, idx_t c_ctx, int running);
char *print_term_to_strbuf(query *q, cell *c, idx_t c_ctx, int running);

ssize_t print_canonical_to_buf(query *q, char *dst, size_t dstlen, cell *c, idx_t c_ctx, int running, unsigned depth);
pl_state print_canonical(query *q, FILE *fp, cell *c, idx_t c_ctx, int running);
char *print_canonical_to_strbuf(query *q, cell *c, idx_t c_ctx, int running);
pl_state print_canonical_to_stream(query *q, stream *str, cell *c, idx_t c_ctx, int running);

