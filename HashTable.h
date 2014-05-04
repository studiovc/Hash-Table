#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <assert.h>
#include <string>

#include "windows.h"

/*
* Compare function
*
*/
static bool StrCompare( const char* first, const char* second )
{
	size_t firstLen = strlen(first);
	size_t secondLen = strlen(second);

	return firstLen == secondLen && !strcmp( first, second );
}


/*
* Hash function
*
*/
static unsigned int ImplHashFunc( const char* buf, int len )
{
	unsigned int hash = 5381;

	while(len--)
	{
		hash = ((hash << 5) + hash) + (*buf++); /* hash * 33 + c */
	}

	return hash;
}


/*
* Hash function
*
*/
static unsigned int ImplCaseHashFunc( const unsigned char* buf, int len )
{
	unsigned int hash = 5381;

	while(len--)
	{
		hash = ((hash << 5) + hash) + tolower(*buf++); /* hash * 33 + c */
	}

	return hash;
}


/*
* Hash function
*
*/
static unsigned int ImplHashFuncSimple( unsigned int key )
{
	key += ~(key << 15);
	key ^=  (key >> 10);
	key +=  (key << 3);
	key ^=  (key >> 6);
	key += ~(key << 11);
	key ^=  (key >> 16);
	return key;
}



/*
* encapsulate the hash table 
* advantage: 
*          good performance;
*          terse interface to make more easy for outstanding and to employ
*
*/
template<class T>
class HashTable
{
public:
	typedef unsigned int (*HashFunctor)( const char* key, int len);
	typedef bool (*KeyCompare)( const char* keyFirst, const char* keySecond );

	static const int INIT_TABLE_SIZE = 689981;	

	typedef struct tagEntryNode
	{
		char*  key;
		T      value;
		tagEntryNode* next;

		tagEntryNode():key(0), value(), next(0)
		{

		}

		tagEntryNode( const char* _key, const T& val ):
			value(val), next(0)
		{
			size_t len = strlen(_key) + 1;
			key = new char[len];
			strncpy( key, _key, len - 1);
			key[len - 1] = '\0';
		}

		~tagEntryNode()
		{
			delete [] key;
			key = 0;
		}

	}EntryNode, *pEntryNode;

	typedef struct tagHashNode
	{
		EntryNode** table;
		size_t      used;
		size_t      size;
		size_t      sizeMask;

		tagHashNode():table(0), used(0), size(0),sizeMask(0)
		{

		}

		tagHashNode( size_t _size ):table(0), used(0), size(_size),
			                        sizeMask(0)
		{
			Init( _size );
			
		}

		~tagHashNode()
		{
		}

		void Init( size_t _size )
		{
			size = _size;
			sizeMask = size - 1;
			table = new EntryNode*[size];
			memset( table, 0x00, sizeof(EntryNode*)*size );
		}


	}HashNode, *pHashNode;


	/*
	*
	*
	*/
	HashTable( HashFunctor functor = ImplHashFunc, KeyCompare cmpFunctor = StrCompare ):m_hashFunctor(functor),
		         m_keyCmpFunctor(cmpFunctor),
		         m_hashTable(new HashNode), m_resizeRatio(2)
	{
		Init( INIT_TABLE_SIZE );
	}

	/*
	*
	*
	*/
	~HashTable()
	{
		Clear();
	}

	/*
	* Clear all node and entity
	*
	*/
	void Clear()
	{
		Clear( m_hashTable );
	}

	/*
	* Inset the pair of key and value 
	*
	*/
	bool Insert( const char* key, const T& value )
	{
		Rehash();

		return Insert( m_hashTable, key, value );
	}

	/*
	* Retrieve the pointer of value for given key
	*
	*/
	T* Find( const char* key )
	{
		unsigned int hash = m_hashFunctor(key, strlen(key));
		unsigned int idx = hash % m_hashTable->size;

		EntryNode* entry = m_hashTable->table[idx];
		while( entry )
		{
			if( m_keyCmpFunctor( entry->key, key) )
			{
				return &entry->value;
			}

			entry = entry->next;
		}

		return NULL;
	}

	/*
	* Delete hashEntry for given key
	*
	*/
	void Delete( const char* key )
	{
		unsigned int hash = m_hashFunctor(key, strlen(key));
		unsigned int idx = hash % m_hashTable->size;

		EntryNode* entry = m_hashTable->table[idx];
		EntryNode* preEntry = 0;
		while( entry )
		{		
			if( m_keyCmpFunctor( entry->key, key ) )
			{
				if( preEntry )
				{
					preEntry->next = entry->next;
				}
				else
				{
					m_hashTable->table[idx] = entry->next;
				}

				delete entry;
				entry = 0;
				m_hashTable->used--;
				return;
			}
			else
			{
				preEntry = entry;
				entry  = entry->next;
			}

		}
	}

protected:
  
	/*
	* Fink the index of corresponding of key value in the table
	*
	*/
	int FindKeyIndex( pHashNode hashNode, const char* key )
	{
		unsigned int hash = m_hashFunctor( key, strlen( key ) );
		unsigned int idx = hash % hashNode->size;

		EntryNode* entry = hashNode->table[idx];
		if( 0 == entry )
			hashNode->used++;

		while( entry )
		{
			if( m_keyCmpFunctor( entry->key, key) )
			{
				return -1;
			}

			entry = entry->next;
		}

		return idx;
	}



	/*
	* Implement insert operation
	*
	*/
	bool Insert( pHashNode hashNode, const char* key, const T& value )
	{
		int idx = FindKeyIndex( hashNode, key );
		if( idx != -1 )
		{
			EntryNode* newNode = new EntryNode( key, value );
			newNode->next = hashNode->table[idx];
			hashNode->table[idx] = newNode;

			return true;
		}

		return false;
	}


	/*
	* Rehash double store memory to make more root then remake insert operation
	* very important
	*/
	void Rehash()
	{
		if( m_hashTable->used >= m_hashTable->size || 
			  (m_hashTable->used > 0 && (m_hashTable->size / m_hashTable->used) < m_resizeRatio ) )
		{
			size_t newSize = NextPrime( m_hashTable->size * 2 );
			pHashNode newHashNode = new HashNode( newSize );

			for( size_t i = 0; i < m_hashTable->size; i++ )
			{
				pEntryNode entryNode = m_hashTable->table[i];
				while( entryNode )
				{
					Insert( newHashNode, entryNode->key, entryNode->value );
					entryNode = entryNode->next;
				}
				
			}

			Clear( m_hashTable );
			m_hashTable = newHashNode;

		}
	}


	/*
	* Implement clear operation
	*
	*/
	void Clear( pHashNode hashNode )
	{
		for( size_t i = 0; i < m_hashTable->size; i++ )
		{
			pEntryNode entryNode = m_hashTable->table[i];
			while( entryNode )
			{
				pEntryNode next = entryNode->next;
				delete entryNode;
				entryNode = next;
			}

		}

		delete [] m_hashTable->table;
	}


	/*
	* Initialization
	*
	*/
	void Init( size_t tableSize )
	{
		m_hashTable->Init( tableSize );
	}


	/*
	* Helper function
	* check prime
	*/
	bool IsPrime( size_t x)
	{
		for( std::size_t i = 3; true; i += 2 )
		{
			std::size_t q = x / i;
			if( q < i )
				return true;

			if( x == q * i )
				return false;
		}
		return true;
	}


	/*
	* Find next prime for given x
	*
	*/
	size_t	NextPrime( size_t x )
	{
		if( x <= 2 )
			return 2;

		if(!(x & 1))
			++x;

		for(; !IsPrime(x); x += 2 );

		return x;
	}

protected:

	HashFunctor m_hashFunctor;

	KeyCompare  m_keyCmpFunctor;

	pHashNode   m_hashTable;

	size_t      m_resizeRatio;



};

/*
* Test hash table
*
*/
void TestHashTable()
{
	unsigned long start = GetTickCount();

	HashTable<int> hashTable;
	const int Len = 500000;
	for( int i = 0; i < Len; i++ )
	{
		char key[16] = {0};
		sprintf(key, "%s_%d", "china", i );

		assert(hashTable.Insert( key, i ));
	}


	for( int i = 0; i < Len; i++ )
	{
		char key[16] = {0};
		sprintf(key, "%s_%d", "china", i );

		if( i > 0 && !(i % 50) )
		{
			hashTable.Delete( key );
			assert( !hashTable.Find( key ) );
		}
		else
		{
			assert(i == *hashTable.Find( key));
		}

		
	}

	unsigned long interval = GetTickCount() - start;
	printf(" hash table consume time is %d \n", interval );
}


/*
* Test STL map
*
*/
void TestHTSTLMap()
{
	unsigned long start = GetTickCount();

	std::map<std::string, int > strMap;
	const int Len = 500000;
	for( int i = 0; i < Len; i++ )
	{
		char key[16] = {0};
		sprintf(key, "%s_%d", "china", i );
		std::string keyStr(key);

		strMap.insert( std::make_pair(keyStr, i )) ;
	}

    std::map<std::string, int >::iterator iter = strMap.begin();
	for( int i = 0; i < Len; i++ )
	{
		char key[16] = {0};
		sprintf(key, "%s_%d", "china", i );
		std::string keyStr(key);

		if( i > 0 && !(i % 50) )
		{
			strMap.erase( key );
			assert( strMap.find( key ) == strMap.end() );
		}
		else
		{
			iter = strMap.find( keyStr );
			assert( iter->second == i );
		}

	}



	unsigned long interval = GetTickCount() - start;
	printf(" STL map consume time is %d \n", interval );
}


/*
* Test suite and compare performance
*
*/
void TestSuiteHashTable()
{
	TestHashTable();
	TestHTSTLMap();

}




#endif 