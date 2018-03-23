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
    std::vector<T> Data;
    int Count = 0;
    int NumNulls = 0;

	null_array()
	{
		Data.resize(cap);
	}

    void push_back(T elem)
    {
        for (int i = 0; i < Count; i++)
        {
            if (Data[i] == NULL)
            {
                Data[i] = elem;
                NumNulls--;
                return;
            }
        }
        Data[Count++] = elem;
    }

	int size() const
	{
		return Count;
	}

    void remove(int i)
    {
        Data[i] = NULL;
        NumNulls++;
    }

    void check_clear()
    {
        if (NumNulls == cap)
        {
            Count = 0;
        }
    }

    typename std::vector<T>::iterator begin()
    {
		return Data.begin();
    }

	typename std::vector<T>::iterator end()
    {
		return Data.begin() + Count;
    }

	T &operator[](int i)
	{
		return Data[i];
	}
};

template<typename T, int cap>
struct fixed_array
{
	std::vector<T> Data;
    int Count = 0;

	fixed_array()
	{
		Data.resize(cap);
	}

    void push_back(T elem)
    {
        if (Count >= cap)
        {
            throw std::runtime_error("fixed array out of memory");
        }
        Data[Count++] = elem;
    }

    int size() const
    {
        return Count;
    }

    void clear()
    {
        Count = 0;
    }

	typename std::vector<T>::iterator begin()
    {
		return Data.begin();
    }

	typename std::vector<T>::iterator end()
    {
		return Data.begin() + Count;
    }

	T &operator[](int i)
	{
		return Data[i];
	}
};

#endif
