// class cCEDictionary
// 8/15/2007 - Alex Zvenigorodsky
//
// Implements a basic word dictionary for searching and iterating.
// 
// The class reads the wordlist from a filename passed in, as well as the maximum word length to map in.
// Separate word on each line.
//
// The words are stored in containers by word length.  
// The dictionary is case insensitive.  (all words are lower case)

#ifndef CEDICTIONARY_H
#define CEDICTIONARY_H

#define SUFFIX_TREE_IMPLEMENTATION

#include "CEString.h"
//#include <hash_set>
#include <set>
#include <list>
//#include <vector>
#include <map>

typedef std::set<cCEString>	tWordList;

// -- FORWARD DECLARATIONS ----------------------------------------------------
class cCEDictionary;

// -- HELPER CLASSES ----------------------------------------------------------
#ifdef SUFFIX_TREE_IMPLEMENTATION
class cWordNode
{
public:
	cWordNode(char c);
	~cWordNode();

	bool 			Contains(char* pWord);				// returns true if this word is contained within this subtree, starting at this node
	bool 			Insert(char* pWord, unsigned long nUsage=1);				// appends the word to the 

	bool			GetAllWordsWithPrefix(char* pPrefix, tWordList& wordList, int nMaxResultsToReturn = -1, bool bIncludeUsage = false);		// if nMaxResultsToReturn is -1 then return all
	cCEString		GetRandomWord();

	// Debugging functions
	void			PrintSubtree(cCEString sPrefix);	// prints all valid words in a subtree (prepends pPrefix to the output)
	void			AnalyzeSubtree();

private:
	cWordNode*		FindChild(char c);			// returns a child node corresponding to the letter, or NULL
	cWordNode*		AddChildNode(char c);		// appends a new child node to the list

	bool			GetAllWordsRecursiveHelper(char* pPrefix, tWordList& wordList, int& nMaxResultsToReturn, bool bIncludeUsage);
	void			AnalyzeSubtreeRecursiveHelper();

	int				CountChildren();

	cWordNode*		mpSibling;
	cWordNode*  	mpChildren;
	unsigned long	mnUsages;
	char			mC;
//	bool			mbValidWordEnding;
};
#endif

// -- MAIN CLASS --------------------------------------------------------------
class cCEDictionary
{
public:
	cCEDictionary();								
	~cCEDictionary();

	bool			Init(unsigned long nMaxWordLength);							// Takes a wordlist filename and the size of the largest word to map in
	void			Shutdown();

	bool			Load(const cCEString& sFilename);					// loads the raw wordlist
	bool			Save(const cCEString& sFilename);					// saves the dictionary
	bool			ComputeUsage(const cCEString& sFilename);			// parses a text file and adds word usage into the dictionary (if the word is already in the raw dictionary)

	// Accessors
	bool			Contains(cCEString sWord);																// copy of the word so that we can check the case-insensitive version
	bool			GetAllWordsWithPrefix(cCEString sPrefix, tWordList& wordList, int nMaxResults = -1, bool bIncludeUsage = false);	// returns all words matching sPrefix.  (if nMaxResults is > 0 then return that many results)
	cCEString		GetRandomWord();

	// Debugging
	void			PrintOutDictionary(unsigned long nWordLength = 0);										// Pass a length of 0 to print out entire dictionary
	void			Analyze();																				// Scans the dictionary for certain stats and dumps it out

protected:
	// Helper functions
	unsigned long	mnMaxWordLength;
	bool			ParseFile(const cCEString& sFilename, bool bOnlyInsertDictionaryWords = true);

#ifdef SUFFIX_TREE_IMPLEMENTATION
	cWordNode*		mpWords;
#else
	tWordList		mWordList;
#endif
};


#endif