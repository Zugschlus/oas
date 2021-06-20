

struct stringlist {
	struct stringlist *next;
	char *text;
};

struct rule {
	unsigned char address[16];
	unsigned char mask[16];
	struct rule *next;
};

struct config {
	struct config *next;
	struct stringlist *pattern;
	struct rule *rule;
};

int oas_address_score(void *address);
