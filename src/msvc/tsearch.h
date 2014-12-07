#ifndef TSEARCH_H
#define TSEARCH_H

void * tsearch(const void *vkey, void **vrootp,
	int(*compar)(const void *, const void *));

void * tfind(const void *vkey, void **vrootp,
	int(*compar)(const void *, const void *));

void * tdelete(const void *vkey, void **vrootp,
	int(*compar)(const void *, const void *));


#endif