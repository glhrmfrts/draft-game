#ifndef DRAFT_MEMORY_H
#define DRAFT_MEMORY_H

struct memory_block
{
    const char *Name;
    memory_block *Prev = NULL;
    void *Base;
    size_t Used;
    size_t Size;
};

struct allocator
{
    virtual ~allocator() {}
};

struct memory_arena : allocator
{
    memory_block *CurrentBlock = NULL;
	bool Free = true;

	~memory_arena();
};

struct memory_pool_entry : allocator
{
    void *Base;
    memory_pool_entry *Next = NULL;
    size_t Used = 0;
};

struct memory_pool
{
    const char *Name;
    memory_arena *Arena = NULL;
    memory_pool_entry *First = NULL;
    memory_pool_entry *FirstFree = NULL;
    size_t ElemSize = 0;
};

template<typename T>
struct generic_pool : memory_pool
{
    generic_pool()
    {
        this->ElemSize = sizeof(T);
        this->Name = typeid(T).name();
    }
};

struct string_format
{
    const char *Format = NULL;
    char *Result = NULL;
	size_t Size = 0;
};

template<typename T, int cap>
struct null_array
{
    std::vector<T> vec;
    int numNulls;

    class iterator
    {
        null_array<T, cap> *arr;
        int i;

        iterator(null_array<T, cap> *arr, int i)
        {
            this->arr = arr;
            while (i < arr->vec.size() && arr->vec[i] == NULL)
            {
                i++;
            }
            this->i = i;
        }

        iterator operator ++()
        {
            return iterator(arr, i++);
        }

        bool operator !=(iterator &rhs)
        {
            return i != rhs.i;
        }

        T &operator *()
        {
            return arr->vec[i];
        }
    };

    void Add(T elem)
    {
        for (int i = 0; i < vec.size(); i++)
        {
            if (vec[i] == NULL)
            {
                vec[i] = elem;
                numNulls--;
                return;
            }
        }
        vec.push_back(elem);
    }

	int Count() const
	{
		return vec.size();
	}

    void Remove(int i)
    {
        vec[i] = NULL;
        numNulls++;
    }

    void CheckClear()
    {
        if (numNulls == cap)
        {
            vec.clear();
        }
    }

    // std::vector aliases
    int size() const
    {
        return vec.size();
    }

    iterator begin()
    {
        return iterator(this, 0);
    }

    iterator end()
    {
        return iterator(this, vec.size() - 1);
    }
};

template<typename T, int cap>
struct fixed_array
{
    std::vector<T> vec;
    int count = 0;

    fixed_array()
    {
        vec.resize(cap);
    }

    void Add(T elem)
    {
        if (count >= vec.size())
        {
            throw std::runtime_error("fixed array out of memory");
        }
        vec[count++] = elem;
    }

    int Count() const
    {
        return count;
    }

    void Clear()
    {
        count = 0;
    }

    // std::vector aliases
    int size() const
    {
        return count;
    }

    typename std::vector<T>::iterator begin()
    {
        return vec.begin();
    }

    typename std::vector<T>::iterator end()
    {
        return vec.begin() + count;
    }
};

#endif
