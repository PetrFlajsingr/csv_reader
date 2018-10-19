//
// Created by Petr Flajsingr on 27/08/2018.
//

#ifndef DATASETS_FIELDS_HEADERS_STRINGFIELD_H_
#define DATASETS_FIELDS_HEADERS_STRINGFIELD_H_

#include "BaseField.h"
#include "Utilities.h"
#include <string>

namespace DataSets {
class StringField : public BaseField {
 protected:
  std::string data;

  void setValue(void *data) override;

 public:
  StringField(const std::string &fieldName,
              BaseDataSet *dataset,
              uint64_t index);

  ValueType getFieldType() override;

  void setAsString(const std::string &value) override;

  std::string getAsString() override;
};
}

#endif //  DATASETS_FIELDS_HEADERS_STRINGFIELD_H_