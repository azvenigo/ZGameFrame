// class cCEDictionary
// 8/15/2007 - Alex Zvenigorodsky
#include "CEDictionary.h"
#include <windows.h>          // for reading files
#include <CEAssert.h>
#include "CEStringHelpers.h"
#include "CEStdDebug.h"
#include <iostream>
#include <algorithm>

const int kMaxWordLength = 32;

#ifdef SUFFIX_TREE_IMPLEMENTATION
static int gTotalNodes = 0;
static int gSingleChildNodes = 0;
static int gValidWords = 0;
static int gINGSuffixes = 0;
static int gEDSuffixes = 0;
static int gSSuffixes = 0;
#endif

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


cCEDictionary::cCEDictionary()
{
#ifdef SUFFIX_TREE_IMPLEMENTATION
	mpWords = NULL;
#endif
}

cCEDictionary::~cCEDictionary()
{
	Shutdown();
}

bool cCEDictionary::Init(unsigned long nMaxWordLength)
{
	CEASSERT(nMaxWordLength < kMaxWordLength);
	mnMaxWordLength = nMaxWordLength;

#ifdef SUFFIX_TREE_IMPLEMENTATION
	if (mpWords)
		delete mpWords;
	mpWords = new cWordNode(0);		// null root node
#endif

	return true;
}

void cCEDictionary::Shutdown()
{
#ifdef SUFFIX_TREE_IMPLEMENTATION
	delete mpWords;
	mpWords = NULL;
#endif
}


bool cCEDictionary::Contains(cCEString sWord)
{
#ifdef SUFFIX_TREE_IMPLEMENTATION
	sWord.MakeLower();

	return mpWords->Contains((char*) sWord.c_str());
#else
	return mWordList.find(sWord) == mWordList.end();
	return find(mWordList.begin(), mWordList.end(), sWord) != mWordList.end();
//	return mWordList.find(sWord) != mWordList.end();
#endif
}



bool cCEDictionary::GetAllWordsWithPrefix(cCEString sPrefix, tWordList& wordList, int nMaxResults, bool bIncludeUsage)
{
	wordList.clear();

#ifdef SUFFIX_TREE_IMPLEMENTATION
	if (mpWords)
	{
		sPrefix.MakeLower();
		mpWords->GetAllWordsWithPrefix((char*) sPrefix.c_str(), wordList, nMaxResults, bIncludeUsage);
	}
#else
	long nPrefixLength = sPrefix.length();
	for (tWordList::iterator it = mWordList.begin(); it != mWordList.end(); it++)
	{
		if ((*it).substr(0, nPrefixLength) == sPrefix)
//			wordList.push_back(*it);
			wordList.insert(*it);
	}
#endif

	return !wordList.empty();		// return true if any words were returned
}

cCEString cCEDictionary::GetRandomWord()
{
#ifdef SUFFIX_TREE_IMPLEMENTATION
	if (mpWords)
		return mpWords->GetRandomWord();

	return "";
#else
	uint32_t nRand = (rand() << 16 | rand())%mWordList.size();
//	return mWordList[ nRand ];
	tWordList::iterator it = mWordList.begin();
	while (nRand-- > 0)
		it++;
	return *it;
#endif
}



void cCEDictionary::PrintOutDictionary(unsigned long /*nWordLength*/)
{
	std::cout << "Printing Entire Dictionary:\n";

#ifdef SUFFIX_TREE_IMPLEMENTATION
	mpWords->PrintSubtree("");
#endif
}

bool cCEDictionary::Load(const cCEString& sFilename)
{
	return ParseFile(sFilename, false);
}

bool cCEDictionary::ComputeUsage(const cCEString& sFilename)
{
	return ParseFile(sFilename, true);
}


bool cCEDictionary::ParseFile(const cCEString& sFilename, bool bOnlyInsertDictionaryWords)
{
	bool bReturn = false;

	HANDLE hFile = CreateFile(sFilename.c_str(), GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
#ifdef _DEBUG
		cCEString sMessage("Failed to open file: " + sFilename );
		CEASSERT_MESSAGE(bReturn, sMessage.c_str());
#endif
		return false;
	}

	DWORD nDummy = 0;
	unsigned long nFileSize = GetFileSize(hFile, &nDummy);

	// Read the entire file into memory for processing faster
	char* pRawBuf = new char[nFileSize+1];
	pRawBuf[nFileSize] = 0;

	unsigned long nNumRead = 0;
	if (ReadFile(hFile, pRawBuf, nFileSize, &nNumRead, NULL))
	{
		if (nNumRead == nFileSize)
		{
			char* pBuf = pRawBuf;
			int64_t nDataSize = 0;
			DecompressDecode(pRawBuf+6, nFileSize-6, pBuf, nDataSize);

			const char* pStartOfWord = NULL;
			pStartOfWord = pBuf;

			unsigned long nWordSize = 0;

			while (pStartOfWord && pStartOfWord < pBuf + nDataSize)
			{
				// Find the next word
				while ((pStartOfWord+nWordSize) && (pStartOfWord+nWordSize) < (pBuf + nDataSize) && !IsAlpha(*(pStartOfWord+nWordSize)))
					nWordSize++;
				pStartOfWord += nWordSize;
				nWordSize = 0;

				// Find the end of the word  (end of file == NULL, or '\n' or '\r')
				while ((pStartOfWord+nWordSize) && (pStartOfWord+nWordSize) < (pBuf + nDataSize) && IsAlpha(*(pStartOfWord+nWordSize)))
					nWordSize++;

				// if the word is of a length we care about
				if (nWordSize >= 3 && nWordSize <= mnMaxWordLength)	// 3 letter words minimum
				{
					cCEString sWord( pStartOfWord, nWordSize );
					sWord.MakeLower();

					if (bOnlyInsertDictionaryWords && !Contains(sWord))	// if we're computing usage, don't insert words that aren't already in the dictionary
					{
						CEDEBUG_OUT("Skipping non-dictionary word \"%s\"\n", sWord.c_str());
						continue;
					}

	#ifdef SUFFIX_TREE_IMPLEMENTATION
					mpWords->Insert((char*) sWord.c_str());
	#else
//					mWordList.push_back(sWord.c_str());
					mWordList.insert(sWord.c_str());
	#endif

				}
			}

			free(pBuf);

			bReturn = true;
		}
	}

	delete[] pRawBuf;
	::CloseHandle(hFile);

#ifdef _DEBUG
	cCEString sMessage("Failed to parse file" + sFilename );
	CEASSERT_MESSAGE(bReturn, sMessage.c_str());
#endif
	return bReturn;
}


bool cCEDictionary::Save(const cCEString& sFilename)
{
	bool bSuccess = false;

#ifdef SUFFIX_TREE_IMPLEMENTATION
	HANDLE hFile = ::CreateFile(sFilename.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		tWordList allWords;

		cCEString sWord;
		for (char c = 'a'; c <= 'z'; c++)
		{
			GetAllWordsWithPrefix(cCEString(c), allWords, -1, true);

			DWORD nNumWritten;
			for (tWordList::iterator it = allWords.begin(); it != allWords.end(); it++)
			{
				sWord = (*it) + "\n";
				WriteFile(hFile, sWord.c_str(), sWord.length(), &nNumWritten, NULL);
			}
		}

		CloseHandle(hFile);

		bSuccess = true;
	}
#else
	CEASSERT(false);
#endif

	return bSuccess;
}


void cCEDictionary::Analyze()
{
#ifdef SUFFIX_TREE_IMPLEMENTATION
	if (mpWords)
		mpWords->AnalyzeSubtree();
#endif
}


#ifdef SUFFIX_TREE_IMPLEMENTATION
cWordNode::cWordNode(char c)
{
	mC					= c;
	mnUsages			= 0;
	mpSibling			= NULL;
	mpChildren			= NULL;

	gTotalNodes++;
}

cWordNode::~cWordNode()
{
	// Call destructor on all children
	cWordNode* pChildNode = mpChildren;
	while (pChildNode)
	{
		cWordNode* pNext = pChildNode->mpSibling;
		delete pChildNode;
		pChildNode = pNext;
	}

	gTotalNodes--;
}

bool cWordNode::Contains(char* pWord)
{
	// If not a valid pointer
	if (pWord == NULL)
		return false;


	if (*pWord == NULL && mnUsages > 0)
		return true;

	cWordNode* pChild = FindChild(*pWord);

	if (!pChild)
		return false;

	return pChild->Contains(pWord+1);
}

bool cWordNode::Insert(char* pWord, unsigned long nUsage)
{
	if (!pWord)
		return false;

	if (*pWord)
	{
		cWordNode* pChild = FindChild(*pWord);

		if (pChild == NULL)
			pChild = AddChildNode(*pWord);

		pChild->Insert(pWord+1, nUsage);
	}
	else
	{
		// If we're inserting a NULL word, that means this node is an end word
		mnUsages += nUsage;
	}

	return true;
}

cWordNode* cWordNode::AddChildNode(char c)
{
	if (mpChildren == NULL)
	{
		mpChildren = new cWordNode(c);
		return mpChildren;
	}

	// Find the last child node (who's sibling is NULL)
	cWordNode* pLastChild = mpChildren;
	while (pLastChild->mpSibling)
	{
		pLastChild = pLastChild->mpSibling;
	}

	// Add a new sibling to that child node and return it
	pLastChild->mpSibling = new cWordNode(c);
	return pLastChild->mpSibling;
}


cWordNode* cWordNode::FindChild(char c)
{
	for (cWordNode* pChild = mpChildren; pChild != NULL; pChild = pChild->mpSibling)
	{
		if (pChild->mC == c)
			return pChild;
	}

	return NULL;
}

cCEString cWordNode::GetRandomWord()
{
	if (mpChildren == NULL)
	{
		CEASSERT(mnUsages>0);
		return cCEString(mC);
	}

	if (mnUsages > 0 && rand() % 3 == 0)
		return cCEString(mC);

	int nRandomSibling = rand()%CountChildren();
	cWordNode* pRandomChild = mpChildren;
	while (nRandomSibling > 0)
	{
		pRandomChild = pRandomChild->mpSibling;
		nRandomSibling--;
	}

	if (mC)
		return cCEString(mC) + pRandomChild->GetRandomWord();

	return pRandomChild->GetRandomWord();
}


void cWordNode::PrintSubtree(cCEString sPrefix)
{
	char buf[4] = {0, 0, 0, 0};
	buf[0] = mC;
	cCEString sPrefixPlusChildChar(sPrefix + (char*) &buf[0]);

	if (mnUsages > 0)
		std::cout << sPrefixPlusChildChar.c_str() << '\n';

	for (cWordNode* pChild = mpChildren; pChild != NULL; pChild = pChild->mpSibling)
	{
		pChild->PrintSubtree(sPrefixPlusChildChar);
	}
}

bool cWordNode::GetAllWordsWithPrefix(char* pPrefix, tWordList& wordList, int nMaxResultsToReturn, bool bIncludeUsage)
{
	if (!pPrefix)
		return false;

	char* pPrefixTemp = pPrefix;

	cWordNode* pNode = this;
	do
	{
		pNode = pNode->FindChild(*pPrefixTemp++);
	} while (pNode && *pPrefixTemp);

	if (!pNode)
		return false;

	// If the prefix is a valid word, add it
	if (pNode->mnUsages > 0)
		wordList.insert(cCEString(pPrefix));

	for (cWordNode* pChild = pNode->mpChildren; pChild != NULL && nMaxResultsToReturn != 0; pChild = pChild->mpSibling)
	{
		std::cout << "Getting all words with prefix \"" << pPrefix << "\" child node: \"" << pChild->mC << "\"\n";
		pChild->GetAllWordsRecursiveHelper(pPrefix, wordList, nMaxResultsToReturn, bIncludeUsage);
	}

	return !wordList.empty();
}

bool inline cWordNode::GetAllWordsRecursiveHelper(char* pPrefix, tWordList& wordList, int& nMaxResultsToReturn, bool bIncludeUsage)
{
	if (nMaxResultsToReturn == 0)
		return true;

	char buf[4] = {0, 0, 0, 0};
	buf[0] = mC;
	cCEString sPrefixPlusChildChar(pPrefix);
	sPrefixPlusChildChar += (char*) &buf[0];

	if (mnUsages>0)
	{
		//wordList.insert(sPrefixPlusChildChar);

//		for (int i = 0; i < 100; i++)
		if (bIncludeUsage)
		{
			if (mnUsages > 1)		// don't return words in the dictionary that aren't used anywhere but the dictionary
			{
				cCEString sWordPlusUsage;
				sWordPlusUsage.Sprintf("%d,%d,%s", mnUsages-1, sPrefixPlusChildChar.length(), sPrefixPlusChildChar.c_str());	// -1 since the dictionary itself doesn't count
				wordList.insert(sWordPlusUsage);
			}
		}
		else
			wordList.insert(sPrefixPlusChildChar);
		nMaxResultsToReturn--;
		//std::cout << "inserting: \"" << sPrefixPlusChildChar.c_str() << "\"\n";
	}

	char* pPrefixPlusChar = (char*) sPrefixPlusChildChar.c_str();
	for (cWordNode* pChild = mpChildren; pChild != NULL && nMaxResultsToReturn != 0; pChild = pChild->mpSibling)
	{
		pChild->GetAllWordsRecursiveHelper(pPrefixPlusChar, wordList, nMaxResultsToReturn, bIncludeUsage);
	}

	return !wordList.empty();
}

void cWordNode::AnalyzeSubtree()
{
	AnalyzeSubtreeRecursiveHelper();

	std::cout << "Total Words: " << gValidWords << "\n";
	std::cout << "Total Nodes: " << gTotalNodes << "\n";
	std::cout << "Total Single Nodes: " << gSingleChildNodes << "\n";
	std::cout << "ING suffixes: " << gINGSuffixes << "\n";
	std::cout << "ED suffixes: " << gEDSuffixes << "\n";
	std::cout << "S suffixes: " << gSSuffixes << "\n";
	std::cout << "Size of Node: " << sizeof(cWordNode) << " bytes.\n";
}

void cWordNode::AnalyzeSubtreeRecursiveHelper()
{
	if (mnUsages>0)
	{
		gValidWords++;

		for (cWordNode* pChild = mpChildren; pChild != NULL; pChild = pChild->mpSibling)
		{
			if (pChild->Contains("ing"))
				gINGSuffixes++;

			if (pChild->Contains("s"))
				gSSuffixes++;

			if (pChild->Contains("ed"))
				gEDSuffixes++;
		}
	}

	// If there's only one child in this node
	if (mpChildren && mpChildren->mpSibling == NULL)
		gSingleChildNodes++;

	for (cWordNode* pChild = mpChildren; pChild != NULL; pChild = pChild->mpSibling)
	{
		pChild->AnalyzeSubtreeRecursiveHelper();
	}
}

int	cWordNode::CountChildren()
{
	int nCount = 0;
	cWordNode* pChild = mpChildren;
	while (pChild)
	{
		nCount++;
		pChild = pChild->mpSibling;
	}

	return nCount;
}

#endif

