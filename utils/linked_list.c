#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include "linked_list.h"

void *get_element(linked_list *ptr, uint32_t index) {
    if (index >= ptr->size) {
        return NULL;
    }
    struct node *current_node = ptr->first;
    for (uint32_t i = 0; i < index; i++) {
        current_node = current_node->next;
    }
    return current_node->value;
}

uint32_t add_first(linked_list *ptr, void *element) {
    node *created = malloc(sizeof(node));
    created->next = ptr->first;
    created->value = element;
    ptr->first->prev = created;
    ptr->first = created;
    return 0;
}

uint32_t add_last(linked_list *ptr, void *element) {
    node *created = malloc(sizeof(node));
    bzero(created, sizeof(node));
    created->prev = ptr->last;
    created->value = element;
    if (ptr->last == NULL) {
        ptr->last = created;
        ptr->first = created;
    } else {
        ptr->last->next = created;
        ptr->last = created;
    }
    return ++ptr->size;
}

linked_list *init_list() {
    linked_list *created = malloc(sizeof(linked_list));
    created->first = NULL;
    created->last = NULL;
    created->size = 0;
    return created;
}

void free_list(linked_list *ptr) {
    node *current = ptr->first;
    if(ptr->size != 0) {
        for (uint32_t i = 1; i < ptr->size; ++i) {
            current = current->next;
            free(current->prev);
        }
        free(current);
    }
    free(ptr);
}

void remove_element(bool (*by)(void *, char *, char *), linked_list *ptr, char *first_to_find, char *second_to_find) {
    node *current = ptr->first;
    while (current != NULL && !by(current->value, first_to_find, second_to_find)) {
        current = current->next;
    }
    if (current != NULL) {
        if (ptr->first == current) {
            ptr->first = current->next;
        }
        if (ptr->last == current) {
            ptr->last = current->prev;
        }
        if (current->prev != NULL) {
            current->prev->next = current->next;
        }
        if (current->next != NULL) {
            current->next->prev = current->prev;
        }
        ptr->size--;
        free(current);
    }
}

void *find_element(bool (*by)(void *, char *, char *), linked_list *ptr, char *first_to_find, char *second_to_find) {
    if (ptr->size == 0) {
        return NULL;
    }
    node *current = ptr->first;
    while (current != NULL && !by(current->value, first_to_find, second_to_find)) {
        current = current->next;
    }
    if (current == NULL) return NULL;
    return current->value;
}

bool by_value(void *value, char *to_find, char *second_argument) {
    return  strcmp(value, to_find) == 0;
}

uint16_t get_last_n(linked_list *ptr, void **buffer, uint16_t buffer_size, bool(*filter)(void *, char*to_filter), char*to_filter) {
    node *current = ptr->last;
    for (uint16_t i = 0; i < buffer_size; ++i) {
        if (current == NULL) return i;
        if (filter == NULL || filter(current->value, to_filter)) {
            buffer[i] = current->value;
        } else {
            --i;
        }
        current = current->prev;
    }
    return buffer_size;
}