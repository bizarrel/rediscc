#include "xmalloc.h"
#include <adlist.h>
#include <cstdint>
#include <optional>

std::optional< adlist* > listCreate( void ) {
  adlist* list;
  if ( ( list = (adlist*)xmalloc( sizeof( *list ) ) ) == nullptr ) {
    return std::nullopt;
  }
  list->head = list->tail = nullptr;
  list->len               = 0;
  list->dup               = nullptr;
  list->free              = nullptr;
  list->match             = nullptr;

  return list;
}

void listRelease( adlist* adlist ) {
  if ( !adlist ) {
    return;
  }
  listEmpty( adlist );
  xfree( adlist );
}

void listEmpty( adlist* adlist ) {
  uint64_t  len;
  listNode *cur, *next;

  cur = adlist->head;
  len = adlist->len;

  while ( len-- ) {
    next = cur->next;
    if ( adlist->free ) adlist->free( cur->value );
    xfree( cur );
    cur = next;
  }
  adlist->head = adlist->tail = nullptr;
  adlist->len                 = 0;
}

std::optional< adlist* > listAddNodeHead( adlist* adlist, void* value ) {
  listNode* node;
  if ( ( node = (listNode*)xmalloc( sizeof( *node ) ) ) == nullptr ) {
    return std::nullopt;
  }
  node->value = value;
  listLinkNodeHead( adlist, node );
  return adlist;
}

std::optional< adlist* > listAddNodeTail( adlist* adlist, void* value ) {
  listNode* node;
  if ( ( node = (listNode*)xmalloc( sizeof( *node ) ) ) == nullptr ) {
    return std::nullopt;
  }
  node->value = value;
  listLinkNodeTail( adlist, node );
  return adlist;
}

void listLinkNodeHead( adlist* adlist, listNode* node ) {
  if ( adlist->len == 0 ) {
    adlist->head = adlist->tail = node;
    node->prev = node->next = nullptr;
  } else {
    node->prev         = nullptr;
    node->next         = adlist->head;
    adlist->head->prev = node;
    adlist->head       = node;
  }
  adlist->len++;
}

void listLinkNodeTail( adlist* adlist, listNode* node ) {
  if ( adlist->len == 0 ) {
    adlist->head = adlist->tail = node;
    node->prev = node->next = nullptr;
  } else {
    node->prev         = adlist->tail;
    node->next         = nullptr;
    adlist->tail->next = node;
    adlist->tail       = node;
  }
  adlist->len++;
}

std::optional< adlist* >
listInsertNode( adlist* adlist, listNode* old_node, void* value, bool after ) {
  listNode* node;
  if ( ( node = (listNode*)xmalloc( sizeof( *node ) ) ) == nullptr ) {
    return std::nullopt;
  }
  node->value = value;

  if ( after ) {
    node->prev = old_node;
    node->next = old_node->next;
    if ( adlist->tail == old_node ) adlist->tail = node;
  } else {
    node->next = old_node;
    node->prev = old_node->prev;
    if ( adlist->head == old_node ) adlist->head = node;
  }

  if ( node->prev != nullptr ) node->prev->next = node;
  if ( node->next != nullptr ) node->next->prev = node;

  adlist->len++;
  return adlist;
}

void listDelNode( adlist* adlist, listNode* node ) {
  listUnlinkNode( adlist, node );
  if ( adlist->free ) adlist->free( node->value );
  xfree( node );
}

void listUnlinkNode( adlist* adlist, listNode* node ) {
  if ( node->prev )
    node->prev->next = node->next;
  else
    adlist->head = node->next;
  if ( node->next )
    node->next->prev = node->prev;
  else
    adlist->tail = node->prev;

  node->next = NULL;
  node->prev = NULL;

  adlist->len--;
}

std::optional< listIter* > listGetIterator( adlist* adlist, IterDirection direction ) {
  listIter* iter;

  if ( ( iter = (listIter*)xmalloc( sizeof( *iter ) ) ) == NULL ) {
    return std::nullopt;
  }
  if ( direction == IterDirection::START_HEAD )
    iter->next = adlist->head;
  else
    iter->next = adlist->tail;
  iter->direction = direction;
  return iter;
}

void listReleaseIterator( listIter* iter ) { xfree( iter ); }

void listRewind( adlist* adlist, listIter* li ) {
  li->next      = adlist->head;
  li->direction = IterDirection::START_HEAD;
}

void listRewindTail( adlist* adlist, listIter* li ) {
  li->next      = adlist->tail;
  li->direction = IterDirection::START_TAIL;
}

std::optional< listNode* > listNext( listIter* iter ) {
  listNode* current = iter->next;

  if ( current != NULL ) {
    if ( iter->direction == IterDirection::START_HEAD )
      iter->next = current->next;
    else
      iter->next = current->prev;
  }
  return current;
}

std::optional< adlist* > listDup( adlist* orig ) {
  adlist*   copy;
  listIter  iter;
  listNode* node;

  if ( ( copy = listCreate().value_or( nullptr ) ) == NULL ) return std::nullopt;
  copy->dup   = orig->dup;
  copy->free  = orig->free;
  copy->match = orig->match;
  listRewind( orig, &iter );
  while ( ( node = listNext( &iter ).value_or( nullptr ) ) != NULL ) {
    void* value;

    if ( copy->dup ) {
      value = copy->dup( node->value );
      if ( value == NULL ) {
        listRelease( copy );
        return std::nullopt;
      }
    } else {
      value = node->value;
    }

    if ( listAddNodeTail( copy, value ).value_or( nullptr ) == NULL ) {
      /* Free value if dup succeed but listAddNodeTail failed. */
      if ( copy->free ) copy->free( value );

      listRelease( copy );
      return std::nullopt;
    }
  }
  return copy;
}

std::optional< listNode* > listSearchKey( adlist* adlist, void* key ) {
  listIter  iter;
  listNode* node;

  listRewind( adlist, &iter );
  while ( ( node = listNext( &iter ).value_or( nullptr ) ) != NULL ) {
    if ( adlist->match ) {
      if ( adlist->match( node->value, key ) ) {
        return node;
      }
    } else {
      if ( key == node->value ) {
        return node;
      }
    }
  }
  return std::nullopt;
}

std::optional< listNode* > listIndex( adlist* adlist, long index ) {
  listNode* n;

  if ( index < 0 ) {
    index = ( -index ) - 1;
    n     = adlist->tail;
    while ( index-- && n )
      n = n->prev;
  } else {
    n = adlist->head;
    while ( index-- && n )
      n = n->next;
  }
  return n;
}

void listRotateTailToHead( adlist* adlist ) {
  if ( list_length( adlist ) <= 1 ) return;

  /* Detach current tail */
  listNode* tail     = adlist->tail;
  adlist->tail       = tail->prev;
  adlist->tail->next = NULL;
  /* Move it as head */
  adlist->head->prev = tail;
  tail->prev         = NULL;
  tail->next         = adlist->head;
  adlist->head       = tail;
}

void listRotateHeadToTail( adlist* adlist ) {
  if ( list_length( adlist ) <= 1 ) return;

  listNode* head     = adlist->head;
  /* Detach current head */
  adlist->head       = head->next;
  adlist->head->prev = NULL;
  /* Move it as tail */
  adlist->tail->next = head;
  head->next         = NULL;
  head->prev         = adlist->tail;
  adlist->tail       = head;
}

void listJoin( adlist* l, adlist* o ) {
  if ( o->len == 0 ) return;

  o->head->prev = l->tail;

  if ( l->tail )
    l->tail->next = o->head;
  else
    l->head = o->head;

  l->tail  = o->tail;
  l->len  += o->len;

  /* Setup other as an empty list. */
  o->head = o->tail = NULL;
  o->len            = 0;
}

void listInitNode( listNode* node, void* value ) {
  node->prev  = NULL;
  node->next  = NULL;
  node->value = value;
}
