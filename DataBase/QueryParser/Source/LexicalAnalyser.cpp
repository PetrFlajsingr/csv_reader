//
// Created by Petr Flajsingr on 2019-02-12.
//

#include <LexicalAnalyser.h>

void DataBase::LexicalAnalyser::setInput(const std::string &newInput) {
  this->input = newInput;
  currentIndex = 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-avoid-goto"

std::tuple<DataBase::Token, std::string, bool> DataBase::LexicalAnalyser::getNextToken() {
  auto it = input.begin() + currentIndex;

  auto state = LexState::start;
  std::string value;
  bool isComment = false;
  bool notLast = true;
  Token token;

  while (it != input.end()) {
    // comment skip
    if (isComment && (*it == '#' || *it == '\0' || *it == '\n' || it == input.end() - 1)) {
      isComment = false;
      it++;
    } else if (*it == '#') {
      isComment = true;
    }
    if (isComment) {
      it++;
      continue;
    }
    //\ comment skip
    switch (state) {
    case LexState::start:
      if (std::isspace(*it)) {
        token = Token::space;
        value = *it;
        goto emit_token_move_iter;
      } else if (isdigit(*it)) {
        state = LexState::num1;
      } else if (*it == '-') {
        state = LexState::negNum;
      } else if (isalpha(*it) || Utilities::isUtf8Accent(*it)) {
        state = LexState::id;
      } else if (*it == '\"') {
        state = LexState::string;
      } else if (*it == '=') {
        token = Token::equal;
        value += *it;
        goto emit_token_move_iter;
      } else if (*it == '(') {
        token = Token::leftBracket;
        value += *it;
        goto emit_token_move_iter;
      } else if (*it == ')') {
        token = Token::rightBracket;
        value += *it;
        goto emit_token_move_iter;
      } else if (*it == '.') {
        token = Token::dot;
        value += *it;
        goto emit_token_move_iter;
      } else if (*it == '|') {
        token = Token::pipe;
        value += *it;
        goto emit_token_move_iter;
      } else if (*it == '*') {
        token = Token::asterisk;
        value += *it;
        goto emit_token_move_iter;
      } else if (*it == ';') {
        token = Token::semicolon;
        value += *it;
        goto emit_token_move_iter;
      } else if (*it == ',') {
        token = Token::colon;
        value += *it;
        goto emit_token_move_iter;
      } else if (*it == '<') {
        state = LexState::less;
      } else if (*it == '>') {
        state = LexState::greater;
      } else if (*it == '!') {
        state = LexState::exclam;
      } else if (*it == '\'') {
        state = LexState::quotedId;
        goto skip_char;
      } else {
        throw LexException(getErrorPrint());
      }
      break;
    case LexState::negNum:
      if (isdigit(*it)) {
        state = LexState::num1;
      } else {
        throw LexException(getErrorPrint());
      }
      break;
    case LexState::num1:
      if (isdigit(*it)) {
        state = LexState::num1;
      } else if (*it == '.') {
        state = LexState::numFloat;
      } else {
        token = Token::number;
        goto emit_token;
      }
      break;
    case LexState::numFloat:
      if (isdigit(*it)) {
        state = LexState::numFloat;
      } else {
        token = Token::numberFloat;
        goto emit_token;
      }
      break;
    case LexState::id:
      if (isalnum(*it) || *it == '_' || *it == '-' || Utilities::isUtf8Accent(*it)) {
        state = LexState::id;
      } else {
        token = keyWordCheck(Utilities::toLower(value));
        goto emit_token;
      }
      break;
    case LexState::string:
      if (*it == '\"') {
        token = Token::string;
        value += *it;
        goto emit_token_move_iter;
      } else {
        state = LexState::string;
      }
      break;
    case LexState::less:
      if (*it == '=') {
        token = Token::lessEqual;
        value += *it;
        goto emit_token_move_iter;
      } else {
        token = Token::less;
        goto emit_token;
      }
    case LexState::greater:
      if (*it == '=') {
        token = Token::greaterEqual;
        value += *it;
        goto emit_token_move_iter;
      } else {
        token = Token::greater;
        goto emit_token;
      }
    case LexState::exclam:
      if (*it == '=') {
        token = Token::notEqual;
        value += *it;
        goto emit_token_move_iter;
      } else {
        throw LexException(getErrorPrint());
      }
    case LexState::quotedId:
      if (*it == '\'') {
        token = Token::id;
        goto emit_token_move_iter;
      } else if (!isalnum(*it) && !isspace(*it) && *it != '_' && *it != '-') {
        throw LexException(getErrorPrint());
      }
    }

    value += *it;
  skip_char:
    it++;
  }
emit_token_move_iter:
  it++;
emit_token:
  if (it == input.end() || *it == '\0') {
    notLast = false;
  }
  if (token == Token::string) {
    value = Utilities::trim(value, "\"");
  }

  currentIndex = std::distance(input.begin(), it);
  return std::make_tuple(token, value, notLast);
}

#pragma clang diagnostic pop

DataBase::Token DataBase::LexicalAnalyser::keyWordCheck(std::string_view str) {
  if (Utilities::compareString(str, "select") == 0) {
    return Token::select;
  } else if (Utilities::compareString(str, "from") == 0) {
    return Token::from;
  } else if (Utilities::compareString(str, "where") == 0) {
    return Token::where;
  } else if (Utilities::compareString(str, "order") == 0) {
    return Token::order;
  } else if (Utilities::compareString(str, "by") == 0) {
    return Token::by;
  } else if (Utilities::compareString(str, "group") == 0) {
    return Token::group;
  } else if (Utilities::compareString(str, "join") == 0) {
    return Token::join;
  } else if (Utilities::compareString(str, "left") == 0) {
    return Token::left;
  } else if (Utilities::compareString(str, "right") == 0) {
    return Token::right;
  } else if (Utilities::compareString(str, "outer") == 0) {
    return Token::outer;
  } else if (Utilities::compareString(str, "having") == 0) {
    return Token::having;
  } else if (Utilities::compareString(str, "sum") == 0) {
    return Token::sum;
  } else if (Utilities::compareString(str, "avg") == 0) {
    return Token::avg;
  } else if (Utilities::compareString(str, "min") == 0) {
    return Token::min;
  } else if (Utilities::compareString(str, "max") == 0) {
    return Token::max;
  } else if (Utilities::compareString(str, "count") == 0) {
    return Token::count;
  } else if (Utilities::compareString(str, "asc") == 0) {
    return Token::asc;
  } else if (Utilities::compareString(str, "desc") == 0) {
    return Token::desc;
  } else if (Utilities::compareString(str, "or") == 0) {
    return Token::logicOr;
  } else if (Utilities::compareString(str, "and") == 0) {
    return Token::logicAnd;
  } else if (Utilities::compareString(str, "on") == 0) {
    return Token::on;
  } else if (Utilities::compareString(str, "as") == 0) {
    return Token::as;
  } else {
    return Token::id;
  }
}

std::string DataBase::LexicalAnalyser::getErrorPrint() {
  std::string fill;
  for (gsl::index i = 0; i < currentIndex; ++i) {
    fill += " ";
  }
  fill += "^";
  return "Lexical error: Character #" + std::to_string(currentIndex) + "\n" + input + "\n" + fill;
}

std::vector<std::tuple<DataBase::Token, std::string, bool>> DataBase::LexicalAnalyser::getAllTokens() {
  std::vector<std::tuple<DataBase::Token, std::string, bool>> tokens;
  do {
    tokens.push_back(getNextToken());
  } while (std::get<2>(tokens.back()));
  return tokens;
}
