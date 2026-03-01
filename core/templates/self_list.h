/**************************************************************************/
/*  self_list.h                                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/error/error_macros.h"
#include "core/templates/sort_list.h"
#include "core/typedefs.h"

template <typename T>
class SelfList {
public:
	class List;

public:
	List *_root = nullptr;
	T *_self = nullptr;
	SelfList<T> *_next = nullptr;
	SelfList<T> *_prev = nullptr;

	class List {
		SelfList<T> *_first = nullptr;
		SelfList<T> *_last = nullptr;

	public:
		void add(SelfList<T> *p_elem) {
			ERR_FAIL_COND(p_elem->_root);

			p_elem->_root = this;
			p_elem->_next = _first;
			p_elem->_prev = nullptr;

			if (_first) {
				_first->_prev = p_elem;

			} else {
				_last = p_elem;
			}

			_first = p_elem;
		}

		void add_last(SelfList<T> *p_elem) {
			ERR_FAIL_COND(p_elem->_root);

			p_elem->_root = this;
			p_elem->_next = nullptr;
			p_elem->_prev = _last;

			if (_last) {
				_last->_next = p_elem;

			} else {
				_first = p_elem;
			}

			_last = p_elem;
		}

		void remove(SelfList<T> *p_elem) {
			ERR_FAIL_COND(p_elem->_root != this);
			if (p_elem->_next) {
				p_elem->_next->_prev = p_elem->_prev;
			}

			if (p_elem->_prev) {
				p_elem->_prev->_next = p_elem->_next;
			}

			if (_first == p_elem) {
				_first = p_elem->_next;
			}

			if (_last == p_elem) {
				_last = p_elem->_prev;
			}

			p_elem->_next = nullptr;
			p_elem->_prev = nullptr;
			p_elem->_root = nullptr;
		}

		void clear() {
			while (_first) {
				remove(_first);
			}
		}

		void sort() {
			sort_custom<Comparator<T>>();
		}

		template <typename C>
		void sort_custom();

		_FORCE_INLINE_ SelfList<T> *first() { return _first; }
		_FORCE_INLINE_ const SelfList<T> *first() const { return _first; }

		// Forbid copying, which has broken behavior.
		void operator=(const List &) = delete;

		_FORCE_INLINE_ ~List() {
			// A self list must be empty on destruction.
			DEV_ASSERT(_first == nullptr);
		}
	};


public:
	_FORCE_INLINE_ bool in_list() const { return _root; }
	_FORCE_INLINE_ void remove_from_list() {
		if (_root) {
			_root->remove(this);
		}
	}
	_FORCE_INLINE_ SelfList<T> *next() { return _next; }
	_FORCE_INLINE_ SelfList<T> *prev() { return _prev; }
	_FORCE_INLINE_ const SelfList<T> *next() const { return _next; }
	_FORCE_INLINE_ const SelfList<T> *prev() const { return _prev; }
	_FORCE_INLINE_ T *self() const { return _self; }

	// Forbid copying, which has broken behavior.
	void operator=(const SelfList<T> &) = delete;

	_FORCE_INLINE_ SelfList(T *p_self) {
		_self = p_self;
	}

	_FORCE_INLINE_ ~SelfList() {
		if (_root) {
			_root->remove(this);
		}
	}
};

template <typename T>
template <typename C>
void SelfList<T>::List::sort_custom() {
	if (_first == _last) {
		return;
	}

	struct Sorter {
		C compare;

		_FORCE_INLINE_ void _connect(SelfList<T> *p_a, SelfList<T> *p_b) {
			p_a->_next = p_b;
			p_b->_prev = p_a;
		}

		_FORCE_INLINE_ void _split(SelfList<T> *p_a, SelfList<T> *p_b) {
			p_a->_next = nullptr;
			p_b->_prev = nullptr;
		}

		SelfList<T> *_get_mid(SelfList<T> *p_head) {
			SelfList<T> *end = p_head;
			SelfList<T> *mid = p_head;
			while (end->_next && end->_next->_next) {
				end = end->_next->_next;
				mid = mid->_next;
			}
			return mid;
		}

		void _merge(SelfList<T> *p_head1, SelfList<T> *p_tail1, SelfList<T> *p_head2, SelfList<T> *p_tail2, SelfList<T> *&r_head, SelfList<T> *&r_tail) {
			if (compare(*(p_head2->_self), *(p_head1->_self))) {
				r_head = p_head2;
				p_head2 = p_head2->_next;
			} else {
				r_head = p_head1;
				p_head1 = p_head1->_next;
			}

			SelfList<T> *curr = r_head;
			while (p_head1 && p_head2) {
				if (compare(*(p_head2->_self), *(p_head1->_self))) {
					_connect(curr, p_head2);
					p_head2 = p_head2->_next;
				} else {
					_connect(curr, p_head1);
					p_head1 = p_head1->_next;
				}
				curr = curr->_next;
			}

			if (p_head1) {
				_connect(curr, p_head1);
				r_tail = p_tail1;
			} else {
				_connect(curr, p_head2);
				r_tail = p_tail2;
			}
		}

		void _merge_sort(SelfList<T> *&r_head, SelfList<T> *&r_tail) {
			if (r_head == r_tail) {
				return;
			}

			SelfList<T> *tail1 = _get_mid(r_head);
			SelfList<T> *head2 = tail1->_next;
			_split(tail1, head2);

			_merge_sort(r_head, tail1);
			_merge_sort(head2, r_tail);
			_merge(r_head, tail1, head2, r_tail, r_head, r_tail);
		}

		bool _is_sorted(SelfList<T> *p_head, SelfList<T> *p_tail, SelfList<T> *&r_sorted_until) {
			r_sorted_until = p_head;
			while (r_sorted_until != p_tail) {
				if (compare(*(r_sorted_until->_next->_self), *(r_sorted_until->_self))) {
					return false;
				}
				r_sorted_until = r_sorted_until->_next;
			}
			return true;
		}

		void sort(SelfList<T> *&r_head, SelfList<T> *&r_tail) {
			SelfList<T> *sorted_until;
			if (_is_sorted(r_head, r_tail, sorted_until)) {
				return;
			}

			SelfList<T> *head_prev = r_head->_prev;
			r_head->_prev = nullptr;
			SelfList<T> *tail_next = r_tail->_next;
			r_tail->_next = nullptr;

			SelfList<T> *head2 = sorted_until->_next;
			_split(sorted_until, head2);
			_merge_sort(head2, r_tail);
			_merge(r_head, sorted_until, head2, r_tail, r_head, r_tail);

			if (head_prev) {
				_connect(head_prev, r_head);
			}
			if (tail_next) {
				_connect(r_tail, tail_next);
			}
		}
	};

	Sorter sorter;
	sorter.sort(_first, _last);
}
