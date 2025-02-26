///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Header file for the Athena Utilities

#include <assert.h>
// #define _CRTDBG_MAP_ALLOC

#pragma once

#include <string.h>

#include <vector>

#include "array1.h"
#include "geom.h"

namespace utl {
class Logger;
}

namespace odb {
class dbBlock;
class dbBox;
class dbInst;
class dbMaster;
class dbNet;
using uint = unsigned int;

// Simple list
template <class T>
class AthList
{
 private:
  struct t_elem
  {
    T m_data;
    t_elem* m_next;
  };
  t_elem* m_start;

 public:
  AthList(void) { m_start = nullptr; };
  ~AthList(void)
  {
    t_elem* i;
    i = m_start;
    while (i) {
      t_elem* next = i->m_next;
      delete i;
      i = next;
    }
  };
  void push(T a)
  {
    t_elem* new_t_elem = new t_elem;
    new_t_elem->m_next = m_start;
    new_t_elem->m_data = a;
    m_start = new_t_elem;
  };

  struct iterator
  {
    AthList<T>* actual_class;
    t_elem* ptr_to_elem;

    iterator next(void)
    {
      ptr_to_elem = ptr_to_elem->m_next;
      return *this;
    }

    T getVal(void) { return ptr_to_elem->m_data; }

    bool end(void)
    {
      if (ptr_to_elem)
        return false;
      else
        return true;
    }
  };
  iterator start(void)
  {
    iterator ret_val;
    ret_val.actual_class = this;
    ret_val.ptr_to_elem = m_start;
    return ret_val;
  }
};

// A class that implements an array that can grow efficiently
template <class T>
class AthArray
{
 private:
  unsigned int m_alloc_size;
  T** m_ptr;
  unsigned int m_num_allocated_elem;
  unsigned int m_num_mallocated_elem;
  unsigned int m_num_mallocated_first_level;
  void find_indexes(unsigned int num,
                    unsigned int& first_level,
                    unsigned int& second_level)
  {
    first_level = num / m_alloc_size;
    second_level = num % m_alloc_size;
  }

 public:
  AthArray(unsigned int alloc_size = 128)
  {
    if (alloc_size < 2)
      alloc_size = 2;
    m_alloc_size = alloc_size;
    m_num_mallocated_first_level = alloc_size;
    m_ptr = (T**) malloc(sizeof(T*) * m_num_mallocated_first_level);
    unsigned int i;
    for (i = 0; i < m_num_mallocated_first_level; i++) {
      m_ptr[i] = nullptr;
    }
    m_num_allocated_elem = 0;
    m_num_mallocated_elem = 0;
  }

  ~AthArray()
  {
    unsigned int i;
    for (i = 0; i < m_num_mallocated_first_level; i++)
      if (m_ptr[i])
        free(m_ptr[i]);
    free(m_ptr);
  }
  void allocNext(unsigned int* ii, unsigned int* jj)
  {
    unsigned int first_level_idx, second_level_idx;
    find_indexes(m_num_allocated_elem, first_level_idx, second_level_idx);
    // reallocate first level if it is too small
    if (m_num_mallocated_first_level * m_alloc_size <= m_num_allocated_elem) {
      int orig_first_level_size = m_num_mallocated_first_level;
      m_num_mallocated_first_level++;
      m_ptr = (T**) realloc(m_ptr, sizeof(T*) * m_num_mallocated_first_level);
      assert(m_ptr);
      for (unsigned int i = orig_first_level_size;
           i < m_num_mallocated_first_level;
           i++) {
        m_ptr[i] = nullptr;
      }
    }
    // Allocate more elements if needed
    if (m_ptr[first_level_idx] == nullptr) {
      unsigned int size = sizeof(T);
      m_ptr[first_level_idx] = (T*) malloc(size * m_alloc_size);
      m_num_mallocated_elem = m_num_mallocated_first_level * m_alloc_size;
    }
    *ii = first_level_idx;
    *jj = second_level_idx;
  }
  void add(void)
  {
    unsigned int first_level_idx, second_level_idx;
    allocNext(&first_level_idx, &second_level_idx);
    // int n= m_num_allocated_elem++;
    m_num_allocated_elem++;
  }
  int add(T elem)
  {
    unsigned int first_level_idx, second_level_idx;

    allocNext(&first_level_idx, &second_level_idx);

    m_ptr[first_level_idx][second_level_idx] = elem;
    int n = m_num_allocated_elem++;
    return n;
  }

  T& operator[](unsigned int idx)
  {
    /*
    if (!(idx < m_num_allocated_elem)) {
            fprintf(stdout, "idx= %d, m_num_allocated_elem= %d\n", idx,
    m_num_allocated_elem);
    }
    */

    assert(idx < m_num_allocated_elem);
    unsigned int first_level_idx, second_level_idx;
    find_indexes(idx, first_level_idx, second_level_idx);
    return m_ptr[first_level_idx][second_level_idx];
  }
  T get(unsigned int idx)
  {
    assert(idx < m_num_allocated_elem);
    unsigned int first_level_idx, second_level_idx;
    find_indexes(idx, first_level_idx, second_level_idx);
    return m_ptr[first_level_idx][second_level_idx];
  }
  unsigned int getLast(void) { return m_num_allocated_elem; }
};

// A simple pool allocation function
//
// Note: T must be default-constructible, as `new` is used to construct T when
// memory debugging is enabled.
template <class T>
class AthPool
{
 private:
  AthArray<T>* m_heap;
  Ath__array1D<T*>* _freeTable;
  AthArray<T*>* _dbgTable;
  bool _memDbg;
  char _className[256];

 public:
  AthPool(bool dbgMem,
          unsigned int freeAllocSize,
          unsigned int alloc_size = 4096)
  {
    m_heap = new AthArray<T>(alloc_size);
    _freeTable = new Ath__array1D<T*>(freeAllocSize);

    _memDbg = false;
    _dbgTable = nullptr;
    if (dbgMem) {
      _dbgTable = new AthArray<T*>(alloc_size);
      _memDbg = true;
    }
  }
  ~AthPool()
  {
    delete m_heap;
    delete _freeTable;

    if (_dbgTable != nullptr)
      delete _dbgTable;
  }

  T* alloc(uint* freeTableFlag = nullptr, uint* id = nullptr)
  {
    T* a = nullptr;

    if (!_memDbg) {
      if (_freeTable->notEmpty()) {
        a = _freeTable->pop();

        if (freeTableFlag != nullptr)
          *freeTableFlag = 1;
      } else {
        m_heap->add();
        uint n = m_heap->getLast() - 1;
        a = &(*m_heap)[n];

        if (id != nullptr)
          *id = n;

        if (freeTableFlag != nullptr)
          *freeTableFlag = 0;
      }
    } else {
      a = new T;
      // TODO replace new with malloc
      // TODO check for error
      assert(a);

      uint n = _dbgTable->add(a);

      if (id != nullptr)
        *id = n;
      if (freeTableFlag != nullptr)
        *freeTableFlag = 1;
    }
    return a;
  }
  void free(T* a)
  {
    assert(a);  // should not free nullptr
    if (_memDbg) {
      delete a;
    } else if (_freeTable != nullptr) {
      _freeTable->add(a);
    }
  }
  T* get(uint ii)
  {
    T* a = &(*m_heap)[ii];
    return a;
  }
  uint getCnt() { return m_heap->getLast(); }
};

// A simple hash implementation
template <class T>
class AthHash
{
  unsigned int* m_listOfPrimes;

  void init_list_of_primes(void)
  {
    m_listOfPrimes = new unsigned int[16];
    m_listOfPrimes[0] = 49978783;
    m_listOfPrimes[1] = 18409199;
    m_listOfPrimes[2] = 1299827;
    m_listOfPrimes[3] = 1176221;
    m_listOfPrimes[4] = 981493;
    m_listOfPrimes[5] = 779377;
    m_listOfPrimes[6] = 530279;
    m_listOfPrimes[7] = 143567;
    m_listOfPrimes[8] = 30389;
    m_listOfPrimes[9] = 6869;
    m_listOfPrimes[10] = 1049;
    m_listOfPrimes[11] = 149;
    m_listOfPrimes[12] = 11;
    m_listOfPrimes[13] = 3;
    m_listOfPrimes[14] = 2;
    m_listOfPrimes[15] = 1;
  }

  // unsigned int m_prime;

  unsigned int l_find_largest_prime_below_number(unsigned int number)
  {
    assert(number > 3);
    int i = 0;
    while (m_listOfPrimes[i] > number)
      i++;
    return m_listOfPrimes[i];
  }

  unsigned int hashFunction(const char* key, unsigned int, unsigned int prime)
  {
    unsigned int hash = 0;
    int c;

    while ((c = *key++) != '\0')
      hash = c + (hash << 6) + (hash << 16) - hash;

    return hash % prime;
  }

  struct t_elem
  {
    const char* key;
    T data;
  };
  int _allocKeyFlag;
  // AthList<t_elem> *m_data;
 public:
  AthList<t_elem>* m_data;
  unsigned int m_prime;

  AthHash(unsigned int size = 100, int store = 1)
  {
    init_list_of_primes();
    m_prime = l_find_largest_prime_below_number(size);
    m_data = new AthList<t_elem>[m_prime];
    _allocKeyFlag = store;
  }

  ~AthHash()
  {
    delete m_listOfPrimes;
    unsigned int i;
    for (i = 0; i < m_prime; i++) {
      typename AthList<t_elem>::iterator iter = m_data[i].start();
      while (!iter.end()) {
        if (_allocKeyFlag > 0)
          free((void*) iter.getVal().key);
        iter.next();
      }
    }
    delete[] m_data;
  }

  void add(const char* key, T data)
  {
    unsigned int hash_val = hashFunction(key, strlen(key), m_prime);
    t_elem new_t_elem;
    new_t_elem.data = data;
    if (_allocKeyFlag > 0)
      new_t_elem.key = strdup(key);
    else
      new_t_elem.key = key;
    m_data[hash_val].push(new_t_elem);
  }

  // Get a stored value. Returns success of failure depending
  // if the value actually is stored or not
  bool get(const char* key, T& data)
  {
    unsigned int hash_val = hashFunction(key, strlen(key), m_prime);
    typename AthList<t_elem>::iterator iter = m_data[hash_val].start();
    while (!iter.end()) {
      if (strcmp(key, iter.getVal().key) == 0) {
        data = iter.getVal().data;
        return true;
      }
      iter.next();
    }
    return false;
  }
  struct iterator
  {
    unsigned int m_first_level_idx;
    typename AthList<t_elem>::iterator m_list_iterator;
    AthHash<T>* m_ptr_to_hash;

    iterator next()
    {
      if (m_list_iterator.end()) {
        m_first_level_idx++;
        m_list_iterator = m_ptr_to_hash->m_data[m_first_level_idx].start();
      } else {
        m_list_iterator.next();
      }
      if (m_list_iterator.end()) {
        assert(m_first_level_idx < m_ptr_to_hash->m_prime);
        return next();
      } else
        return *this;
    }

    bool end()
    {
      if (m_first_level_idx >= m_ptr_to_hash->m_prime)
        return true;
      if (m_first_level_idx < m_ptr_to_hash->m_prime - 1)
        return false;
      return m_list_iterator.end();
    }

    T getVal(void) { return m_list_iterator.getVal().data; }

    char* getKey(void) { return m_list_iterator.getVal().key; }
  };

  iterator start(void)
  {
    iterator tmp_iter;
    unsigned int i;
    for (i = 0; i < m_prime; i++) {
      tmp_iter.m_list_iterator = m_data[i].start();
      if (!tmp_iter.m_list_iterator.end())
        break;
    }
    tmp_iter.m_first_level_idx = i;
    tmp_iter.m_ptr_to_hash = this;
    return tmp_iter;
  }
};

class RUDYCalculator
{
 public:
  class Tile
  {
   public:
    odb::Rect getRect() const { return rect_; }
    void setRect(int lx, int ly, int ux, int uy);
    void addRUDY(float rudy);
    float getRUDY() const { return rudy_; }

   private:
    odb::Rect rect_;
    float rudy_ = 0;
  };

  explicit RUDYCalculator(dbBlock* block);

  /**
   * \pre we need to call this function after `setGridConfig` and
   * `setWireWidth`.
   * */
  void calculateRUDY();

  /**
   * Set the grid area and grid numbers.
   * Default value will be the die area of block and (40, 40), respectively.
   * */
  void setGridConfig(odb::Rect block, int tileCntX, int tileCntY);

  /**
   * Set the wire length for calculate RUDY.
   * If the layer which name is metal1 and it has getWidth value, then this
   * function will not applied, but it will apply that information.
   * */
  void setWireWidth(int wireWidth) { wireWidth_ = wireWidth; }

  const Tile& getTile(int x, int y) const { return grid_.at(x).at(y); }
  std::pair<int, int> getGridSize() const;

 private:
  /**
   * \pre This function should be called after `setGridConfig`
   * */
  void makeGrid();
  Tile& getEditableTile(int x, int y) { return grid_.at(x).at(y); }
  void processMacroObstruction(odb::dbMaster* macro, odb::dbInst* instance);
  void processIntersectionGenericObstruction(odb::Rect obstruction_rect,
                                             int tile_width,
                                             int tile_height,
                                             int nets_per_tile);
  void processIntersectionSignalNet(odb::Rect net_rect,
                                    int tile_width,
                                    int tile_height);

  dbBlock* block_;
  odb::Rect gridBlock_;
  int tileCntX_ = 40;
  int tileCntY_ = 40;
  int wireWidth_ = 100;
  const int pitches_in_tile_ = 15;

  std::vector<std::vector<Tile>> grid_;
};

int makeSiteLoc(int x, double site_width, bool at_left_from_macro, int offset);

void cutRows(dbBlock* block,
             const int min_row_width,
             const std::vector<dbBox*>& blockages,
             int halo_x,
             int halo_y,
             utl::Logger* logger);

// Generates a string with the macro placement in mpl2 input format for
// individual macro placement
std::string generateMacroPlacementString(dbBlock* block);

class WireLengthEvaluator
{
 public:
  WireLengthEvaluator(dbBlock* block) : block_(block) {}
  int64_t hpwl() const;

 private:
  int64_t hpwl(dbNet* net) const;

  dbBlock* block_;
};

}  // namespace odb
