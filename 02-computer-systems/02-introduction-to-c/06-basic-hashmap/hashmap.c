#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STARTING_BUCKETS 8
#define NODE_EMPTY 0
#define NODE_OCCUPIED 1
#define NODE_DELETED -1

#define MAX_KEY_SIZE 10

typedef struct Map_node {
    void *key;
    size_t key_len;
    void *value;
    size_t hash;
    int8_t status;
} Map_node;

typedef struct Hashmap {
    size_t cap;
    Map_node *arr;

    size_t (*hash)(const void *);

    size_t size;
} Hashmap;


Hashmap *Hashmap_new(size_t (*const hash)(const void *)) {
    Hashmap *const map = malloc(sizeof(Hashmap));
    map->cap = STARTING_BUCKETS;
    map->arr = calloc(sizeof(Map_node), STARTING_BUCKETS);
    map->hash = hash;
    map->size = 0;
    return map;
}

void Hashmap_free(Hashmap *const map) {
    if (NULL == map) return;

    for (size_t i = 0; i < map->cap; ++i)
        if (NODE_OCCUPIED == map->arr[i].status) free(map->arr[i].key);
    free(map->arr);
    free(map);
}

size_t next_power_of_2(size_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return ++n;
}

void Hashmap_print(const Hashmap *const map) {
    for (int i = 0; i < map->cap; ++i) {
        printf("{k=%s, v=%d, h=%lu, s= %d}\n", (char *) map->arr[i].key, *(int *) map->arr[i].value, map->arr[i].hash,
               map->arr[i].status);
    }
    fflush(stdout);
}

#define NEED_RESIZE -1
#define HAS_NEXT 0
#define FOUND 1
#define REPLACE 2

int8_t probing(const Hashmap *const map, const void *const key, const size_t key_len, const size_t pos,
               const size_t hash) {
    if (NODE_EMPTY == map->arr[pos].status) return FOUND;

    if (hash == map->arr[pos].hash) {
        if (NODE_DELETED == map->arr[pos].status) return FOUND;
        if (key_len == map->arr[pos].key_len
            && !memcmp(key, map->arr[pos].key, key_len))
            return REPLACE;
    } else return NEED_RESIZE;

    return HAS_NEXT;
}

void node_set(Map_node *const node, const void *const key, const size_t key_len, void *const value, const size_t hash) {
    if (NODE_OCCUPIED == node->status)
        free(node->key);
    node->key_len = key_len;
    node->key = malloc(key_len);
    memcpy(node->key, key, key_len);
    node->value = value;
    node->hash = hash;
    node->status = NODE_OCCUPIED;
}

void Hashmap_set(Hashmap *const map, const void *const key, const size_t key_len, void *const value) {
    const size_t hv = map->hash(key) % map->cap;
    int8_t flg;
    if ((flg = probing(map, key, key_len, hv, hv)) >= FOUND) {
        node_set(map->arr + hv, key, key_len, value, hv);
        if (FOUND == flg)
            map->size++;
        return;
    }

    if (HAS_NEXT == flg) {
        const size_t n = next_power_of_2(map->cap);
        for (size_t i = 1; i <= n && HAS_NEXT == flg; ++i) {
            const size_t t = (hv + (i + i * i) >> 1) % n;
            if (t >= map->cap) continue;
            if ((flg = probing(map, key, key_len, t, hv)) >= FOUND) {
                node_set(map->arr + t, key, key_len, value, hv);
                if (FOUND == flg)
                    map->size++;
                return;
            }
        }
    }

    Map_node *const old_arr = map->arr;
    size_t const old_cap = map->cap;

    map->cap <<= 1;
    map->arr = calloc(sizeof(Map_node), map->cap);
    map->size = 0;

    for (size_t i = 0; i < old_cap; ++i) {
        if (NODE_OCCUPIED == old_arr[i].status) {
            Hashmap_set(map, old_arr[i].key, old_arr[i].key_len, old_arr[i].value);
            free(old_arr[i].key);
        }
    }
    free(old_arr);

    Hashmap_set(map, key, key_len, value);
}

#define STOP -1
#define CONTINUE 0
#define FOUND 1

int8_t probing2(const Hashmap *const map, const void *const key, const size_t key_len, const size_t pos,
                const size_t hash) {
    if (NODE_EMPTY == map->arr[pos].status) return STOP;
    if (hash != map->arr[pos].hash)return STOP;
    if (NODE_DELETED == map->arr[pos].status
        || key_len != map->arr[pos].key_len
        || memcmp(key, map->arr[pos].key, key_len))
        return CONTINUE;
    return FOUND;
}

void *node_get(const Map_node *const node) {
    return node->value;
}

void *Hashmap_get(const Hashmap *const map, const void *key, const size_t key_len) {
    const size_t hv = map->hash(key) % map->cap;
    int8_t ret;
    if (FOUND == (ret = probing2(map, key, key_len, hv, hv)))
        return node_get(map->arr + hv);

    if (CONTINUE == ret) {
        const size_t n = next_power_of_2(map->cap);
        for (size_t i = 1; i <= n && CONTINUE == ret; ++i) {
            const size_t t = (hv + (i + i * i) >> 1) % n;
            if (t >= map->cap) continue;
            if (FOUND == (ret = probing2(map, key, key_len, t, hv)))
                return node_get(map->arr + t);
        }
    }

    return NULL;
}

void *node_delete(Map_node *const node) {
    node->status = NODE_DELETED;
    free(node->key);
    node->key_len = 0;
    return node->value;
}

void *Hashmap_delete(Hashmap *const map, const void *key, const size_t key_len) {
    const size_t hv = map->hash(key) % map->cap;
    int8_t ret;
    if (FOUND == (ret = probing2(map, key, key_len, hv, hv))) {
        map->size--;
        return node_delete(map->arr + hv);
    }

    if (CONTINUE == ret) {
        const size_t n = next_power_of_2(map->cap);
        for (size_t i = 1; i <= n && CONTINUE == ret; ++i) {
            const size_t t = (hv + (i + i * i) >> 1) % n;
            if (t >= map->cap) continue;
            if (FOUND == (ret = probing2(map, key, key_len, t, hv))) {
                map->size--;
                return node_delete(map->arr + t);
            }
        }
    }
    return NULL;
}

size_t djb2(const void *key) {
    const char *_key = key;
    unsigned long hash = 5381;
    int c;
    while (c = *(_key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

int main() {

    Hashmap *h = Hashmap_new(djb2);

    // basic get/set functionality
    int a = 5;
    float b = 7.2;
    Hashmap_set(h, "item a", strlen("item a") + 1, &a);
    Hashmap_set(h, "item b", strlen("item b") + 1, &b);
    assert(Hashmap_get(h, "item a",strlen("item a")+1) == &a);
    assert(Hashmap_get(h, "item b",strlen("item b")+1) == &b);

    // using the same key should override the previous value
    int c = 20;
    Hashmap_set(h, "item a", strlen("item a") + 1, &c);
    assert(Hashmap_get(h, "item a",strlen("item a")+1) == &c);

    // basic delete functionality
    Hashmap_delete(h, "item a", strlen("item a") + 1);
    assert(Hashmap_get(h, "item a",strlen("item a")+1) == NULL);

    // handle collisions correctly
    // note: this doesn't necessarily test expansion
    int i, n = STARTING_BUCKETS * 1000, ns[n];
    char key[MAX_KEY_SIZE];
    for (i = 0; i < n; i++) {
        ns[i] = i;
        sprintf(key, "item %d", i);
        Hashmap_set(h, key, strlen(key) + 1, &ns[i]);
    }
    for (i = 0; i < n; i++) {
        sprintf(key, "item %d", i);
        assert(Hashmap_get(h, key, strlen(key)+1) == &ns[i]);
    }

    Hashmap_free(h);
    /*
       stretch goals:
       - expand the underlying array if we start to get a lot of collisions
       - support non-string keys
       - try different hash functions
       - switch from chaining to open addressing
       - use a sophisticated rehashing scheme to avoid clustered collisions
       - implement some features from Python dicts, such as reducing space use,
       maintaing key ordering etc. see https://www.youtube.com/watch?v=npw4s1QTmPg
       for ideas
       */
    printf("ok\n");
}
