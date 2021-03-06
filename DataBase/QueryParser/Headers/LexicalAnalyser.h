//
// Created by Petr Flajsingr on 2019-02-12.
//

#ifndef PROJECT_LEXICALANALYSER_H
#define PROJECT_LEXICALANALYSER_H

#include <QueryCommon.h>
#include <QueryException.h>
#include <Utilities.h>
#include <gsl/gsl>
#include <string>
#include <tuple>
#include <vector>

namespace DataBase {

/**
 * Analyse and tokenize query inputs.
 */
class LexicalAnalyser {
public:
  /**
   *
   * @param newInput string to tokenize
   */
  void setInput(const std::string &newInput);
  /**
   * Parse another token
   * @return parsed token, string form of the token, true if this is not last token, false otherwise
   */
  std::tuple<Token, std::string, bool> getNextToken();
  /**
   * Tokenize the entire input
   * @return same as getNextToken() @see LexicalAnalyser::getNextToken()
   */
  std::vector<std::tuple<Token, std::string, bool>> getAllTokens();

private:
  std::string input;

  gsl::index currentIndex;

  static Token keyWordCheck(std::string_view str);

  std::string getErrorPrint();

  enum class LexState { start, negNum, num1, numFloat, id, string, exclam, less, greater, quotedId };
};
} // namespace DataBase

#endif // PROJECT_LEXICALANALYSER_H
