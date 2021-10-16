/*
 Author: José Bollo <jobol@nonadev.net>

 https://gitlab.com/jobol/mustach

 SPDX-License-Identifier: ISC
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <malloc.h>
#endif

#include "mustach.h"
//#include "mustach-extract.h"

#if !defined(INCLUDE_PARTIAL_EXTENSION)
# define INCLUDE_PARTIAL_EXTENSION ".mustache"
#endif

typedef struct mustach_extract mustach_extract_t;

/* global hook for partials */
int (*mustach_extract_get_partial)(const char *name, struct mustach_sbuf *sbuf) = NULL;

enum kind {
	K_nil, /* nothing */
	K_str, /* string */
	K_val, /* value of key */
	K_esc, /* value of key escaped */
	K_if1, /* enter if true */
	K_if0  /* enter if false */
};

struct node {
	struct node *next;
	struct node *parent;
	struct node *child;
	size_t length;
	enum kind kind;
	char data[];
};

/* internal structure for extracting */
struct extract {
	struct node *root;
	struct node *parent;
};

/* length given by masking with 3 */
enum comp {
	C_no = 0,
	C_eq = 1,
	C_lt = 5,
	C_le = 6,
	C_gt = 9,
	C_ge = 10
};

enum sel {
	S_none = 0,
	S_ok = 1,
	S_objiter = 2,
	S_ok_or_objiter = S_ok | S_objiter
};


static struct node *mknode(enum kind kind, size_t length, const char *data)
{
	struct node *r = malloc(sizeof *r + length + 1);
	if (r) {
		r->next = r->parent = r->child = 0;
		r->length = length;
		r->kind = kind;
		memcpy(r->data, data, length);
		r->data[length] = 0;
	}
	return r;
}

static struct node *reverse(struct node *head, struct node *prev)
{
	while(head) {
		struct node *next = head->next;
		head->next = prev;
		prev = head;
		head = next;
	}
	return prev;
}

static int forall(struct node *head, int (*fun)(struct node *item, void *closure), void *closure)
{
	int rc = 0;
	while (!rc && head) {
		rc = fun(head, closure);
		head = head->next;
	}
	return rc;
}

static void forallrec(struct node *head, void (*fun)(struct node *item, void *closure, int pos), void *closure)
{
	int level = 1;
	while (head && level) {
		if (head->child) {
			fun(head, closure, -1);
			level++;
			head = head->child;
		}
		else {
			fun(head, closure, 0);
			while (!head->next) {
				head = head->parent;
				level--;
				if (!level || !head)
					return;
				fun(head, closure, 1);
			}
			head = head->next;
		}
	}
}

static void reverserec_cb(struct node *item, void *closure, int pos)
{
	if (pos < 0 && item->child)
		item->child = reverse(item->child, 0);
}

static struct node *reverserec(struct node *head)
{
	struct node *result = reverse(head, 0);
	forallrec(result, reverserec_cb, 0);
	return result;
}

#if 0
static enum comp getcomp(char *head, int sflags)
{
	return (head[0] == '=' && (sflags & Mustach_With_Equal)) ? C_eq
		: (head[0] == '<' && (sflags & Mustach_With_Compare)) ? (head[1] == '=' ? C_le : C_lt)
		: (head[0] == '>' && (sflags & Mustach_With_Compare)) ? (head[1] == '=' ? C_ge : C_gt)
		: C_no;
}

static char *keyval(char *head, int sflags, enum comp *comp)
{
	char *w, car, escaped;
	enum comp k;

	k = C_no;
	w = head;
	car = *head;
	escaped = (sflags & Mustach_With_EscFirstCmp) && (getcomp(head, sflags) != C_no);
	while (car && (escaped || (k = getcomp(head, sflags)) == C_no)) {
		if (escaped)
			escaped = 0;
		else
			escaped = ((sflags & Mustach_With_JsonPointer) ? car == '~' : car == '\\')
			    && (getcomp(head + 1, sflags) != C_no);
		if (!escaped)
			*w++ = car;
		head++;
		car = *head;
	}
	*w = 0;
	*comp = k;
	return k == C_no ? NULL : &head[k & 3];
}

static char *getkey(char **head, int sflags)
{
	char *result, *iter, *write, car;

	car = *(iter = *head);
	if (!car)
		result = NULL;
	else {
		result = write = iter;
		if (sflags & Mustach_With_JsonPointer)
		{
			while (car && car != '/') {
				if (car == '~')
					switch (iter[1]) {
					case '1': car = '/'; /*@fallthrough@*/
					case '0': iter++;
					}
				*write++ = car;
				car = *++iter;
			}
			*write = 0;
			while (car == '/')
				car = *++iter;
		}
		else
		{
			while (car && car != '.') {
				if (car == '\\' && (iter[1] == '.' || iter[1] == '\\'))
					car = *++iter;
				*write++ = car;
				car = *++iter;
			}
			*write = 0;
			while (car == '.')
				car = *++iter;
		}
		*head = iter;
	}
	return result;
}

static enum sel sel(struct extract *w, const char *name)
{
	enum sel result;
	int i, j, sflags, scmp;
	char *key, *value;
	enum comp k;

	/* make a local writeable copy */
	size_t lenname = 1 + strlen(name);
	char buffer[lenname];
	char *copy = buffer;
	memcpy(copy, name, lenname);

	/* check if matches json pointer selection */
	sflags = w->flags;
	if (sflags & Mustach_With_JsonPointer) {
		if (copy[0] == '/')
			copy++;
		else
			sflags ^= Mustach_With_JsonPointer;
	}

	/* extract the value, translate the key and get the comparator */
	if (sflags & (Mustach_With_Equal | Mustach_With_Compare))
		value = keyval(copy, sflags, &k);
	else {
		k = C_no;
		value = NULL;
	}

	/* case of . alone if Mustach_With_SingleDot? */
	if (copy[0] == '.' && copy[1] == 0 /*&& (sflags & Mustach_With_SingleDot)*/)
		/* yes, select current */
		result = w->itf->sel(w->closure, NULL) ? S_ok : S_none;
	else
	{
		/* not the single dot, extract the first key */
		key = getkey(&copy, sflags);
		if (key == NULL)
			return 0;

		/* select the root item */
		if (w->itf->sel(w->closure, key))
			result = S_ok;
		else if (key[0] == '*'
		      && !key[1]
		      && !value
		      && !*copy
		      && (w->flags & Mustach_With_ObjectIter)
		      && w->itf->sel(w->closure, NULL))
			result = S_ok_or_objiter;
		else
			result = S_none;
		if (result == S_ok) {
			/* iterate the selection of sub items */
			key = getkey(&copy, sflags);
			while(result == S_ok && key) {
				if (w->itf->subsel(w->closure, key))
					/* nothing */;
				else if (key[0] == '*'
				      && !key[1]
				      && !value
				      && !*copy
				      && (w->flags & Mustach_With_ObjectIter))
					result = S_objiter;
				else
					result = S_none;
				key = getkey(&copy, sflags);
			}
		}
	}
	/* should it be compared? */
	if (result == S_ok && value) {
		if (!w->itf->compare)
			result = S_none;
		else {
			i = value[0] == '!';
			scmp = w->itf->compare(w->closure, &value[i]);
			switch (k) {
			case C_eq: j = scmp == 0; break;
			case C_lt: j = scmp < 0; break;
			case C_le: j = scmp <= 0; break;
			case C_gt: j = scmp > 0; break;
			case C_ge: j = scmp >= 0; break;
			default: j = i; break;
			}
			if (i == j)
				result = S_none;
		}
	}
	return result;
}

static int write(struct extract *w, const char *buffer, size_t size, FILE *file)
{
	int r;

	if (w->writecb)
		r = w->writecb(file, buffer, size);
	else
		r = fwrite(buffer, 1, size, file) == size ? MUSTACH_OK : MUSTACH_ERROR_SYSTEM;
	return r;
}

static int emit(void *closure, const char *buffer, size_t size, int escape, FILE *file)
{
	struct extract *w = closure;
	int r;
	size_t s, i;
	char car;

	if (w->emitcb)
		r = w->emitcb(file, buffer, size, escape);
	else if (!escape)
		r = write(w, buffer, size, file);
	else {
		i = 0;
		r = MUSTACH_OK;
		while(i < size && r == MUSTACH_OK) {
			s = i;
			while (i < size && (car = buffer[i]) != '<' && car != '>' && car != '&' && car != '"')
				i++;
			if (i != s)
				r = write(w, &buffer[s], i - s, file);
			if (i < size && r == MUSTACH_OK) {
				switch(car) {
				case '<': r = write(w, "&lt;", 4, file); break;
				case '>': r = write(w, "&gt;", 4, file); break;
				case '&': r = write(w, "&amp;", 5, file); break;
				case '"': r = write(w, "&quot;", 6, file); break;
				}
				i++;
			}
		}
	}
	return r;
}

static int getoptional(struct extract *w, const char *name, struct mustach_sbuf *sbuf)
{
	enum sel s = sel(w, name);
	if (!(s & S_ok))
		return 0;
	return w->itf->get(w->closure, sbuf, s & S_objiter);
}

static int get(void *closure, const char *name, struct mustach_sbuf *sbuf)
{
	struct extract *w = closure;
	if (getoptional(w, name, sbuf) <= 0)
		sbuf->value = "";
	return MUSTACH_OK;
}
#endif

static void prt(struct node *item, void *closure, int pos)
{
	int *level = closure;
	*level -= pos > 0;
	int n = *level;
	while(n--) printf(" ");
	if (pos < 0)
		printf("BEGIN ");
	else if (pos > 0)
		printf("END ");
	switch(item->kind) {
	case K_val: printf("K %s", item->data); break;
	case K_esc: printf("E %s", item->data); break;
	case K_if1: printf("# %s", item->data); break;
	case K_if0: printf("^ %s", item->data); break;
	case K_str:
		printf("= ");
		char *p = item->data;
		while(*p) {
			switch(*p) {
			case '\n': printf("\\n"); break;
			case '\t': printf("\\t"); break;
			case '\r': printf("\\r"); break;
			case '\v': printf("\\v"); break;
			case '\f': printf("\\f"); break;
			default: printf("%c", *p); break;
			}
			p++;
		}
		break;
	default:
	case K_nil: break;
	}
	printf("\n");
	*level += pos < 0;
}

static void dump(struct node *root)
{
	int n=0;
	forallrec(root, prt, &n);
}

static struct node *addnode(struct extract *x, enum kind kind, size_t length, const char *data)
{
	struct node *n = mknode(kind, length, data);
	if (n) {
		n->parent = x->parent;
		if (n->parent) {
			n->next = n->parent->child;
			n->parent->child = n;
		}
//printf("\n----\n");dump(x->root);
	}
	return n;
}

static int enter(void *closure, const char *name, int expected)
{
	struct extract *x = closure;
	struct node *n = addnode(x, expected ? K_if1 : K_if0, strlen(name), name);
	if (!n)
		return MUSTACH_ERROR_SYSTEM;
	x->parent = n;
	return expected ? 1 : -1;
}

static int next(void *closure)
{
	return 0;
}

static int leave(void *closure)
{
	struct extract *x = closure;
	x->parent = x->parent->parent;
	return MUSTACH_OK;
}

static int put(void *closure, const char *name, int escape, FILE *file)
{
	struct extract *x = closure;
	struct node *n = addnode(x, escape ? K_esc : K_val, strlen(name), name);
	return n ? MUSTACH_OK : MUSTACH_ERROR_SYSTEM;
}

static int emit(void *closure, const char *buffer, size_t size, int escape, FILE *file)
{
	struct extract *x = closure;
	struct node *n = addnode(x, K_str, size, buffer);
	return n ? MUSTACH_OK : MUSTACH_ERROR_SYSTEM;
}












static int get_partial_from_file(const char *name, struct mustach_sbuf *sbuf)
{
	static char extension[] = INCLUDE_PARTIAL_EXTENSION;
	size_t s;
	long pos;
	FILE *file;
	char *path, *buffer;

	/* allocate path */
	s = strlen(name);
	path = malloc(s + sizeof extension);
	if (path == NULL)
		return MUSTACH_ERROR_SYSTEM;

	/* try without extension first */
	memcpy(path, name, s + 1);
	file = fopen(path, "r");
	if (file == NULL) {
		memcpy(&path[s], extension, sizeof extension);
		file = fopen(path, "r");
	}
	free(path);

	/* if file opened */
	if (file == NULL)
		return MUSTACH_ERROR_PARTIAL_NOT_FOUND;

	/* compute file size */
	if (fseek(file, 0, SEEK_END) >= 0
	 && (pos = ftell(file)) >= 0
	 && fseek(file, 0, SEEK_SET) >= 0) {
		/* allocate value */
		s = (size_t)pos;
		buffer = malloc(s + 1);
		if (buffer != NULL) {
			/* read value */
			if (1 == fread(buffer, s, 1, file)) {
				/* force zero at end */
				sbuf->value = buffer;
				buffer[s] = 0;
				sbuf->freecb = free;
				fclose(file);
				return MUSTACH_OK;
			}
			free(buffer);
		}
	}
	fclose(file);
	return MUSTACH_ERROR_SYSTEM;
}

static int partial(void *closure, const char *name, struct mustach_sbuf *sbuf)
{
	struct extract *w = closure;
	int rc;
	if (mustach_extract_get_partial != NULL)
		rc = mustach_extract_get_partial(name, sbuf);
	else
		rc = get_partial_from_file(name, sbuf);
	if (rc != MUSTACH_OK)
		sbuf->value = "";
	return MUSTACH_OK;
}

const struct mustach_itf mustach_extract_itf = {
	.start = NULL,
	.put = put,
	.enter = enter,
	.next = next,
	.leave = leave,
	.partial = partial,
	.get = NULL,
	.emit = emit,
	.stop = NULL
};

int mustach_extract(const char *template, size_t length, int flags, mustach_extract_t **result)
{
	int rc;
	struct extract x;
	x.root = x.parent = mknode(K_nil, 0, 0);
	if (!x.parent)
		rc = MUSTACH_ERROR_SYSTEM;
	else
		rc = mustach_file(template, length, &mustach_extract_itf, &x, flags, 0);
dump(reverserec(x.root));
	*result = 0;
	return rc;
}















#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>

static const size_t BLOCKSIZE = 8192;

static const char *errors[] = {
	"??? unreferenced ???",
	"system",
	"unexpected end",
	"empty tag",
	"tag too long",
	"bad separators",
	"too depth",
	"closing",
	"bad unescape tag",
	"invalid interface",
	"item not found",
	"partial not found"
};

static const char *errmsg = 0;
static int flags = 0;
static FILE *output = 0;

static void help(char *prog)
{
	char *name = basename(prog);
#define STR(x) #x
	printf("%s version %s\n", name, STR(VERSION));
#undef STR
	printf("usage: %s json-file mustach-templates...\n", name);
	exit(0);
}

static char *readfile(const char *filename, size_t *length)
{
	int f;
	struct stat s;
	char *result;
	size_t size, pos;
	ssize_t rc;

	result = NULL;
	if (filename[0] == '-' &&  filename[1] == 0)
		f = dup(0);
	else
		f = open(filename, O_RDONLY);
	if (f < 0) {
		fprintf(stderr, "Can't open file: %s\n", filename);
		exit(1);
	}

	fstat(f, &s);
	switch (s.st_mode & S_IFMT) {
	case S_IFREG:
		size = s.st_size;
		break;
	case S_IFSOCK:
	case S_IFIFO:
		size = BLOCKSIZE;
		break;
	default:
		fprintf(stderr, "Bad file: %s\n", filename);
		exit(1);
	}

	pos = 0;
	result = malloc(size + 1);
	do {
		if (result == NULL) {
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		rc = read(f, &result[pos], (size - pos) + 1);
		if (rc < 0) {
			fprintf(stderr, "Error while reading %s\n", filename);
			exit(1);
		}
		if (rc > 0) {
			pos += (size_t)rc;
			if (pos > size) {
				size = pos + BLOCKSIZE;
				result = realloc(result, size + 1);
			}
		}
	} while(rc > 0);

	close(f);
	if (length != NULL)
		*length = pos;
	result[pos] = 0;
	return result;
}

static int load_json(const char *filename);
static int process(const char *content, size_t length);
static void close_json();

int main(int ac, char **av)
{
	char *t, *f;
	char *prog = *av;
	int s;
	size_t length;

	(void)ac; /* unused */
	flags = Mustach_With_AllExtensions;
	output = stdout;

	if (*++av) {
		if (!strcmp(*av, "-h") || !strcmp(*av, "--help"))
			help(prog);
		f = (av[0][0] == '-' && !av[0][1]) ? "/dev/stdin" : av[0];
		s = load_json(f);
		if (s < 0) {
			fprintf(stderr, "Can't load json file %s\n", av[0]);
			if(errmsg)
				fprintf(stderr, "   reason: %s\n", errmsg);
			exit(1);
		}
		while(*++av) {
			t = readfile(*av, &length);
			s = process(t, length);
			free(t);
			if (s != MUSTACH_OK) {
				s = -s;
				if (s < 1 || s >= (int)(sizeof errors / sizeof * errors))
					s = 0;
				fprintf(stderr, "Template error %s (file %s)\n", errors[s], *av);
			}
		}
		close_json();
	}
	return 0;
}

static int load_json(const char *filename)
{ return 0; }
static int process(const char *template, size_t length)
{
	mustach_extract_t *p;
	return mustach_extract(template, length, flags, &p);
}
static void close_json()
{}

