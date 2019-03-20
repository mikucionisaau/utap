/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -C -E -t -L C++ -c -Z Keywords keywords.gperf  */
struct Keyword { char *name; int token; int32_t syntax; };
/* maximum key range = 70, duplicates = 0 */

class Keywords
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct Keyword *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
Keywords::hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72,  0, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 40, 30, 10,
       0, 15,  5,  5, 20, 20, 72,  5, 72, 15,
      25,  0, 10,  0, 30,  0,  0,  5, 72, 10,
      72, 10, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
      72, 72, 72, 72, 72, 72
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

const struct Keyword *
Keywords::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 33,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 28,
      MIN_HASH_VALUE = 2,
      MAX_HASH_VALUE = 71
    };

  static const struct Keyword wordlist[] =
    {
      {""}, {""},
      {"do", 			T_DO,		SYNTAX_NEW},
      {""},
      {"quit",			T_QUIT,		SYNTAX_PROPERTY},
      {"trans",			T_TRANS,	SYNTAX_OLD | SYNTAX_NEW},
      {"struct",			T_STRUCT,	SYNTAX_NEW},
      {"default", 		T_DEFAULT,	SYNTAX_NEW},
      {""}, {""},
      {"guard",			T_GUARD,	SYNTAX_OLD | SYNTAX_NEW},
      {"urgent",			T_URGENT,	SYNTAX_OLD | SYNTAX_NEW},
      {"typedef",		T_TYPEDEF,	SYNTAX_NEW},
      {"deadlock",		T_DEADLOCK,	SYNTAX_PROPERTY},
      {"sync",			T_SYNC,		SYNTAX_OLD | SYNTAX_NEW},
      {"const",			T_CONST,	SYNTAX_OLD | SYNTAX_NEW},
      {"commit",			T_COMMIT,	SYNTAX_OLD | SYNTAX_NEW},
      {"process",		T_PROCESS,	SYNTAX_OLD | SYNTAX_NEW},
      {""},
      {"true",			T_TRUE,		SYNTAX_NEW | SYNTAX_PROPERTY},
      {"state",			T_STATE,	SYNTAX_OLD | SYNTAX_NEW},
      {"system",			T_SYSTEM,	SYNTAX_OLD | SYNTAX_NEW},
      {""}, {""},
      {"init",			T_INIT,		SYNTAX_OLD | SYNTAX_NEW},
      {"false",			T_FALSE,	SYNTAX_NEW | SYNTAX_PROPERTY},
      {"switch", 		T_SWITCH,	SYNTAX_NEW},
      {"if", 			T_IF,		SYNTAX_NEW},
      {"not",			T_BOOL_NOT,	SYNTAX_PROPERTY},
      {"case", 			T_CASE,		SYNTAX_NEW},
      {"while",			T_WHILE,	SYNTAX_NEW},
      {""},
      {"or",			T_BOOL_OR,	SYNTAX_PROPERTY},
      {"continue", 		T_CONTINUE,	SYNTAX_NEW},
      {"else", 			T_ELSE,		SYNTAX_NEW},
      {"imply",			T_IMPLY,	SYNTAX_PROPERTY},
      {""}, {""},
      {"for",			T_FOR,		SYNTAX_NEW},
      {""},
      {"break",			T_BREAK,	SYNTAX_NEW},
      {""}, {""},
      {"and",			T_BOOL_AND,	SYNTAX_PROPERTY},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"broadcast       \011T_BROADCAST",	SYNTAX_NEW},
      {""}, {""},
      {"return", 		T_RETURN,	SYNTAX_NEW},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"assign",			T_ASSIGN,	SYNTAX_OLD | SYNTAX_NEW}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return 0;
}

