#include "includes.h"
#include "oas.h"

static struct rule *rules = NULL;
static struct config *conf = NULL;
static char *cmdline = NULL;

static void suffix2mask(int suffix, unsigned char *mask)
{
	char mhs[50];
	int restbitcount = suffix;
	// initialize each byte with 0
	// >= 8 set all bits
	// 7-1 use mapped value
	// < 1 do nothing, keep the  initial 0
	// 8 bits handled, reduce restbits
	unsigned char bit2val[] = { 0, 128, 192, 224, 240, 248, 252, 254, 255 };
	for (int i = 0; i < 16; i++) {
		mask[i] = 0;
		if (restbitcount >= 8) {
			mask[i] = 255;
		} else if (restbitcount > 0) {
			mask[i] = bit2val[restbitcount];
		}
		restbitcount -= 8;
		sprintf(&mhs[2 * i], "%02x", mask[i]);
	}
	syslog(LOG_DEBUG, "%d: %s", suffix, mhs);
}

static char *to02x(const unsigned char *in, int len)
{
	char *retval = (char *)malloc(sizeof(char) * len * 2 + 1);
	for (int i = 0; i < len; i++) {
		sprintf(&retval[2 * i], "%02x", in[i]);
	}
	retval[2 * len] = 0;
	return retval;
}

static int empty(const char *line)
{
	while (*line) {
		if (!isspace(*line))
			return 0;
		line++;
	}
	return 1;
}
static struct stringlist *append_stringlist(struct stringlist *sl, char *s)
{
	syslog(LOG_DEBUG, "append_stringlist: %p %s", sl, s);
	struct stringlist *new =
		(struct stringlist *)calloc(1, sizeof(struct stringlist));
	new->text = s;
	new->next = NULL;
	if (sl == NULL) {
		sl = new;
	} else {
		struct stringlist *iterator = sl;
		syslog(LOG_DEBUG, "append_stringlist: %p %p", iterator,
		       iterator->next);
		while (iterator->next != NULL)
			iterator = iterator->next;
		iterator->next = new;
	}
	return sl;
}
static struct rule *append_rule(struct rule *rl, struct rule *new)
{
	if (rl == NULL) {
		rl = new;
	} else {
		struct rule *iterator = rl;
		syslog(LOG_DEBUG, "append_rule: %p %p", iterator,
		       iterator->next);
		while (iterator->next != NULL) {
			syslog(LOG_DEBUG, "loop: %p %p", iterator,
			       iterator->next);

			iterator = iterator->next;
		}
		iterator->next = new;
	}
	return rl;
}

static void log_stringlist(struct stringlist *sl)
{
	struct stringlist *iter = sl;
	while (iter != NULL) {
		syslog(LOG_DEBUG, "stringlist: %p %s  %p", iter, iter->text,
		       iter->next);
		iter = iter->next;
	}
}

static void log_config(struct config *conf)
{
	struct config *iter = conf;
	while (iter != NULL) {
		syslog(LOG_DEBUG, "config: %p %p %p %p", iter, iter->pattern,
		       iter->rule, iter->next);
		log_stringlist(iter->pattern);
		iter = iter->next;
	}
}

static void log_rule(struct rule *rl)
{
	struct rule *r = rl;
	while (r != NULL) {
		syslog(LOG_DEBUG, "rule: %p %s/%s %p", r, to02x(r->address, 16),
		       to02x(r->mask, 16), r->next);
		if (r == r->next) {
			syslog(LOG_WARNING, "rule->next points to rule! p=%p",
			       r);
			break;
		}
		r = r->next;
	}
}
static void read_cmdline()
{
	FILE *fd;

	fd = fopen("/proc/self/cmdline", "r");
	if (fd == NULL) {
		syslog(LOG_ERR, "cannot read /proc/self/cmdline");
		return;
	}
	int maxlen = 250;
	cmdline = (char *)calloc(maxlen + 1, sizeof(char));
	int c;
	int pos = 0;
	int del_last = 0;
	while ((c = getc(fd)) >= 0) {
		del_last = 0;
		if (c == 0) {
			c = ' ';
			del_last = 1;
		}
		if (pos <= maxlen) {
			cmdline[pos++] = c;
		}
	}
	if (del_last == 1)
		cmdline[pos - 1] = 0;
}

static int cmdline_match(char *expr)
{
	unsigned char *logexpr = (unsigned char *)expr;
	int retval = -999;
	if (cmdline == NULL) {
		read_cmdline();
	}
	regex_t *compiled = calloc(1, sizeof(regex_t));
	regcomp(compiled, expr, REG_EXTENDED | REG_NOSUB);
	retval = regexec(compiled, cmdline, 0, NULL, 0);
	syslog(LOG_DEBUG, "cmdline_match(): >>%s<< %s expression >>%s<< ",
	       (unsigned char *)cmdline, retval ? "does not match" : "matches",
	       logexpr);

	return retval;
}
static struct config *readconfig()
{
	// it is a one shot
	// if failures were fixed between two connect()
	// bad luck, restart programm
	struct config *retval =
		(struct config *)calloc(1, sizeof(struct config));
	retval->next = NULL;
	retval->pattern = NULL;
	retval->rule = NULL;
	char *cfgfilename = getenv("OAS_CONF_F");
	if (cfgfilename == NULL) {
		syslog(LOG_WARNING,
		       "enviroment does not contain Variable OAS_CONF_F");
		return retval;
	}
	FILE *fd = fopen(cfgfilename, "r");
	if (fd == NULL) {
		syslog(LOG_WARNING, "Can not open file:  %s ", cfgfilename);
		return retval;
	}
	char line[1000];
	int expect = 1;
	struct config *current = retval;
	while (fgets(line, 1000, fd) != NULL) {
		if (line[0] == '#') {
			continue;
		}
		if (empty(line)) {
			continue;
		}
		if (strncmp("cmdlines:", line, 9) == 0) {
			if (current->pattern != NULL) {
				current->next = (struct config *)calloc(
					1, sizeof(struct config));
				current = current->next;
				current->next = NULL;
				current->pattern = NULL;
				current->rule = NULL;
			}
			expect = 1;
			continue;
		}
		if (strncmp("addresses:", line, 10) == 0) {
			expect = 0;
			continue;
		}
		if (expect == 1) {
			current->pattern = append_stringlist(
				current->pattern,
				strndup(line, strlen(line) - 1));
			syslog(LOG_DEBUG, "pattern: %s", line);
		}
		if (expect == 0) {
			struct rule *next =
				(struct rule *)calloc(1, sizeof(struct rule));
			next->next = NULL;
			char *s_addr = strtok(line, "/");
			char *s_mask = strtok(NULL, "/");
			syslog(LOG_DEBUG, "address: %s split: %s and %s",
			       line, s_addr, s_mask);
			if (inet_pton(AF_INET6, s_addr, next->address) != 1) {
				continue;
			}
			if (inet_pton(AF_INET6,
				      strndup(s_mask, strlen(s_mask) - 1),
				      next->mask) != 1) {
				int suffix = atoi(s_mask);
				if (suffix > 0)
					suffix2mask(suffix, next->mask);
				else {
					syslog(LOG_WARNING,
					       "readconfig(): failed to parse address mask: %s",
					       s_mask);
					free(next);
					continue;
				}
			}
			current->rule = append_rule(current->rule, next);

			log_rule(current->rule);
		}
	}
	log_config(retval);
	return retval;
}

static void get_rules()
{
	if (rules != NULL)
		return;
	if (conf == NULL)
		conf = readconfig();
	if (conf == NULL) {
		syslog(LOG_WARNING,
		       "Something wrong with readconfig(). Using dummy rules!");
		rules = malloc(sizeof(struct rule));
		for (int i = 0; i < 16; i++) {
			rules->address[i] = 255;
			rules->mask[i] = 255;
		}
		return;
	}

	struct config *iterator_conf = conf;
	while (iterator_conf != NULL && rules == NULL /* first match only */) {
		struct stringlist *pattern = iterator_conf->pattern;
		while (pattern != NULL) {
			if (cmdline_match(pattern->text) == 0) {
				rules = append_rule(rules, iterator_conf->rule);
				break; /* avoid loop in the rule-list if
					 more than one pattern in
					 the same pattern-list match */
			}
			pattern = pattern->next;
		}
		iterator_conf = iterator_conf->next;
	}
	log_rule(rules);
}

int oas_address_score(void *addr)
{
	int retval = 10000;
	if (addr != NULL) {
		unsigned char *caddr = addr;
		get_rules();
		struct rule *iter = rules;
		int rulenum = 1;
		while (iter != NULL && retval == 10000) {
			syslog(LOG_DEBUG,
			       "oas_address_score testing pattern %s/%s for %s",
			       to02x(iter->address, 16), to02x(iter->mask, 16),
			       addr != NULL ? to02x(addr, 16) : "NULL");
			int l;
			for (l = 0; l < 16; l++) {
				unsigned char leftside =
					(iter->address[l]) & (iter->mask[l]);
				unsigned char rightside =
					caddr[l] & (iter->mask[l]);
				if (leftside != rightside)
					break;
			}
			if (l == 16)
				retval = rulenum;
			rulenum++;
			if (iter == iter->next) {
				syslog(LOG_WARNING,
				       "endless loop detected, breaking out!");
				log_rule(rules);
				break;
			}
			iter = iter->next;
		}
	}
	syslog(LOG_DEBUG, "oas_address_score returns %d for %s", retval,
	       addr != NULL ? to02x(addr, 16) : "NULL");
	return retval;
}
