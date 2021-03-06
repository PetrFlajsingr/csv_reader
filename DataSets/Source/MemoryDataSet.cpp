//
// Created by Petr Flajsingr on 25/08/2018.
//

#include <FieldFactory.h>
#include <MemoryDataSet.h>
#include <MemoryViewDataSet.h>

void DataSets::MemoryDataSet::open(DataProviders::BaseDataProvider &dataProvider,
                                   const std::vector<ValueType> &fieldTypes) {
  if (isOpen) {
    throw IllegalStateException("Dataset is already open.");
  }
  createFields(dataProvider.getHeader(), fieldTypes);
  data.emplace_back(new DataSetRow());
  data.emplace_back(new DataSetRow());
  loadData(dataProvider);
  data.shrink_to_fit();

  isOpen = true;
}

void DataSets::MemoryDataSet::openEmpty(const std::vector<std::string> &fieldNames,
                                        const std::vector<ValueType> &fieldTypes) {
  if (isOpen) {
    throw IllegalStateException("Dataset is already open.");
  }
  createFields(fieldNames, fieldTypes);
  data.emplace_back(new DataSetRow());
  data.emplace_back(new DataSetRow());
  isOpen = true;
}

void DataSets::MemoryDataSet::loadData(DataProviders::BaseDataProvider &dataProvider) {
  data.pop_back();
  while (dataProvider.next()) {
    addRecord(dataProvider);
  }
  data.emplace_back(new DataSetRow());
}

void DataSets::MemoryDataSet::createFields(std::vector<std::string> columns, std::vector<ValueType> types) {
  if (columns.size() != types.size()) {
    throw InvalidArgumentException(std::string("Column size doesn't match type size in table " + getName()).c_str());
  }
  for (gsl::index i = 0; i < static_cast<gsl::index>(columns.size()); ++i) {
    fields.emplace_back(FieldFactory::CreateField(columns[i], i, types[i], this));
  }
  columnCount = fields.size();
}

void DataSets::MemoryDataSet::addRecord(DataProviders::BaseDataProvider &dataProvider) {
  auto trimQuotes = [] (const std::string &value) {
    if (value.front() == '"') {
      return value.substr(1, value.length() - 2);
    }
    return value;
  };
  auto record = dataProvider.getRow();
  data.emplace_back(new DataSetRow());

  data.back()->reserve(columnCount);
  for (gsl::index i = 0; i < static_cast<gsl::index>(record.size()); ++i) {
    switch (fields[i]->getFieldType()) {
    case ValueType::Integer:
      data.back()->emplace_back(DataContainer{._integer = Utilities::stringToInt(trimQuotes(record[i]))});
      break;
    case ValueType::Double:
      data.back()->emplace_back(DataContainer{._double = Utilities::stringToDouble(record[i])});
      break;
    case ValueType::String:
      data.back()->emplace_back(DataContainer{._string = strdup(record[i].c_str())});
      break;
    case ValueType::Currency:
      data.back()->emplace_back(DataContainer{._currency = new Currency(record[i])});
      break;
    case ValueType::DateTime:
      data.back()->emplace_back(DataContainer{._dateTime = new DateTime(record[i], DateTimeType::Date)});
      break;
    default:
      throw IllegalStateException("Internal error. DataSets::MemoryDataSet::addRecord");
    }
  }
}

void DataSets::MemoryDataSet::close() {
  isOpen = false;
  if (!data.empty()) {
    for (auto level1 : data) {
      gsl::index iter = 0;
      for (auto level2 : *level1) {
        if (fields[iter]->getFieldType() == ValueType::String) {
          delete level2._string;
        } else if (fields[iter]->getFieldType() == ValueType::Currency) {
          delete level2._currency;
        } else if (fields[iter]->getFieldType() == ValueType::DateTime) {
          delete level2._dateTime;
        }
        iter++;
      }
      level1->clear();
      delete level1;
    }
    data.clear();
  }
}

void DataSets::MemoryDataSet::setFieldValues() {
  for (size_t i = 0; i < fields.size(); i++) {
    auto currentCell = getCell(currentRecord, i);
    switch (fields[i]->getFieldType()) {
    case ValueType::Integer:
      setFieldData(fields[i].get(), &currentCell._integer);
      break;
    case ValueType::Double:
      setFieldData(fields[i].get(), &currentCell._double);
      break;
    case ValueType::String:
      setFieldData(fields[i].get(), currentCell._string);
      break;
    case ValueType::Currency:
      setFieldData(fields[i].get(), currentCell._currency);
      break;
    case ValueType::DateTime:
      setFieldData(fields[i].get(), currentCell._dateTime);
      break;
    default:
      throw IllegalStateException("Internal error.");
    }
  }
}

void DataSets::MemoryDataSet::first() {
  currentRecord = getFirst();
  setFieldValues();
}

void DataSets::MemoryDataSet::last() {
  currentRecord = getLast();
  setFieldValues();
}

bool DataSets::MemoryDataSet::next() {
  currentRecord++;
  if (currentRecord >= static_cast<gsl::index>(data.size()) - 1) {
    return false;
  }
  setFieldValues();
  return true;
}

bool DataSets::MemoryDataSet::previous() {
  currentRecord--;
  if (currentRecord <= 0) {
    return false;
  }
  setFieldValues();
  return true;
}

void DataSets::MemoryDataSet::sort(SortOptions &options) {
  gsl::index fieldCount = fields.size();
  bool isInRange = std::all_of(options.options.begin(), options.options.end(),
                               [fieldCount](SortItem &item) { return item.field->getIndex() < fieldCount; });
  if (!isInRange) {
    throw InvalidArgumentException("Field index is out of bounds");
  }

  std::vector<std::function<int8_t(const DataSetRow *, const DataSetRow *)>> compareFunctions;

  std::transform(options.options.begin(), options.options.end(), std::back_inserter(compareFunctions),
                 [](const SortItem &option) { return option.field->getCompareFunction(); });

  auto optionArray = options.options;
  auto compareFunction = [&optionArray, &compareFunctions](const DataSetRow *a, const DataSetRow *b) {
    for (gsl::index i = 0; i < static_cast<gsl::index>(optionArray.size()); ++i) {
      int compareResult = compareFunctions[i](a, b);
      if (compareResult < 0) {
        return optionArray[i].order == SortOrder::Ascending;
      } else if (compareResult > 0) {
        return optionArray[i].order == SortOrder::Descending;
      }
    }
    return false;
  };

  std::sort(data.begin() + 1, data.end() - 1, compareFunction);

  currentRecord = 0;
}

std::shared_ptr<DataSets::ViewDataSet> DataSets::MemoryDataSet::filter(const FilterOptions &options) {
  auto fieldNames = getFieldNames();
  std::vector<ValueType> fieldTypes;
  std::vector<std::pair<int, int>> fieldIndices;
  for (const auto &field : fields) {
    fieldTypes.emplace_back(field->getFieldType());
    fieldIndices.emplace_back(0, field->getIndex());
  }
  auto resultView = std::make_shared<MemoryViewDataSet>(getName() + "_filtered", fieldNames, fieldTypes, fieldIndices);
  resultView->data.emplace_back();

  for (const auto &iter : data) {
    if (iter->empty()) {
      continue;
    }
    bool valid = true;

    for (const auto &filter : options.options) {
      if (!valid) {
        break;
      }
      auto cell = (*iter)[filter.field->getIndex()];

      if (filter.field->getFieldType() == ValueType::String) {
        std::string toCompare(cell._string);
        for (const auto &search : filter.searchData) {
          switch (filter.filterOption) {
          case FilterOption::Equals:
            valid = Utilities::compareString(toCompare, search._string) == 0;
            break;
          case FilterOption::StartsWith:
            valid = std::strncmp(toCompare.c_str(), search._string, strlen(search._string)) == 0;
            break;
          case FilterOption::Contains:
            valid = toCompare.find(search._string) != std::string::npos;
            break;
          case FilterOption::EndsWith:
            valid = Utilities::endsWith(toCompare, search._string);
            break;
          case FilterOption::NotContains:
            valid = toCompare.find(search._string) == std::string::npos;
            break;
          case FilterOption::NotStartsWith:
            valid = std::strncmp(toCompare.c_str(), search._string, strlen(search._string)) != 0;
            break;
          case FilterOption::NotEndsWith:
            valid = !Utilities::endsWith(toCompare, search._string);
            break;
          default:
            throw std::runtime_error("MemoryDataSet::Filter(): invalid filterOption");
          }
          if (valid) {
            break;
          }
        }

      } else if (filter.field->getFieldType() == ValueType::Integer) {
        auto toCompare = cell._integer;
        for (const auto &search : filter.searchData) {
          valid = Utilities::compareInt(toCompare, search._integer) == 0;
          if (valid) {
            break;
          }
        }
      } else if (filter.field->getFieldType() == ValueType::Double) {
        auto toCompare = cell._double;
        for (const auto &search : filter.searchData) {
          valid = Utilities::compareDouble(toCompare, search._double) == 0;
          if (valid) {
            break;
          }
        }
      } else if (filter.field->getFieldType() == ValueType::Currency) {
        auto toCompare = cell._currency;
        for (const auto &search : filter.searchData) {
          valid = Utilities::compareCurrency(*toCompare, *search._currency) == 0;
          if (valid) {
            break;
          }
        }
      } else if (filter.field->getFieldType() == ValueType::DateTime) {
        auto toCompare = cell._dateTime;
        for (const auto &search : filter.searchData) {
          valid = Utilities::compareDateTime(*toCompare, *search._dateTime) == 0;
          if (valid) {
            break;
          }
        }
      }
    }

    if (valid) {
      resultView->data.emplace_back(std::vector<DataSetRow *>{iter});
    }
  }
  resultView->data.emplace_back();
  return resultView;
}

DataSets::BaseField *DataSets::MemoryDataSet::fieldByName(std::string_view name) const {
  for (auto &field : fields) {
    if (Utilities::compareString(field->getName(), name) == 0) {
      return field.get();
    }
  }
  std::string errMsg = "Field named \"" + std::string(name) + "\" not found. DataSets::MemoryDataSet::fieldByName";
  throw InvalidArgumentException(errMsg.c_str());
}

DataSets::BaseField *DataSets::MemoryDataSet::fieldByIndex(gsl::index index) const { return fields.at(index).get(); }

bool DataSets::MemoryDataSet::isLast() const { return currentRecord >= getLast(); }

DataSets::MemoryDataSet::~MemoryDataSet() { close(); }

void DataSets::MemoryDataSet::setData(void *newData, gsl::index index, ValueType type) {
  switch (type) {
  case ValueType::Integer:
    getCell(currentRecord, index)._integer = *reinterpret_cast<int *>(newData);
    break;
  case ValueType::Double:
    getCell(currentRecord, index)._double = *reinterpret_cast<int *>(newData);
    break;
  case ValueType::String:
    delete[] getCell(currentRecord, index)._string;
    getCell(currentRecord, index)._string = reinterpret_cast<gsl::zstring<>>(newData);
    break;
  case ValueType::Currency:
    *(getCell(currentRecord, index)._currency) = *(reinterpret_cast<Currency *>(newData));
    break;
  case ValueType::DateTime:
    *(getCell(currentRecord, index)._dateTime) = *(reinterpret_cast<DateTime *>(newData));
    break;
  default:
    throw IllegalStateException("Invalid value type.");
  }
}

void DataSets::MemoryDataSet::append() {
  data.pop_back();
  data.emplace_back(new DataSetRow());
  for (auto &field : fields) {
    switch (field->getFieldType()) {
    case ValueType::Integer:
      data.back()->emplace_back(DataContainer{._integer = 0});
      break;
    case ValueType::Double:
      data.back()->emplace_back(DataContainer{._double = 0});
      break;
    case ValueType::String:
      data.back()->emplace_back(DataContainer{._string = nullptr});
      break;
    case ValueType::Currency:
      data.back()->emplace_back(DataContainer{._currency = new Currency()});
      break;
    case ValueType::DateTime:
      data.back()->emplace_back(DataContainer{._dateTime = new DateTime()});
      break;
    default:
      throw IllegalStateException("Internal error DataSets::MemoryDataSet::append().");
    }
  }
  data.emplace_back(new DataSetRow());
  last();
}

void DataSets::MemoryDataSet::append(DataProviders::BaseDataProvider &dataProvider) {
  if (!isOpen) {
    throw IllegalStateException("Dataset is not open.");
  }
  loadData(dataProvider);
  data.shrink_to_fit();
}

DataSets::MemoryDataSet::MemoryDataSet(std::string_view dataSetName) : BaseDataSet(dataSetName) {}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

std::vector<DataSets::BaseField *> DataSets::MemoryDataSet::getFields() const {
  std::vector<BaseField *> result;
  for (const auto &field : fields) {
    result.emplace_back(field.get());
  }
  return result;
}

std::vector<std::string> DataSets::MemoryDataSet::getFieldNames() const {
  std::vector<std::string> result;
  std::transform(fields.begin(), fields.end(), std::back_inserter(result),
                 [](const std::shared_ptr<BaseField> &field) { return std::string(field->getName()); });
  return result;
}

std::vector<ValueType> DataSets::MemoryDataSet::getFieldTypes() const {
  std::vector<ValueType> result;
  std::transform(fields.begin(), fields.end(), std::back_inserter(result),
                 [](const std::shared_ptr<BaseField> &field) { return field->getFieldType(); });
  return result;
}


bool DataSets::MemoryDataSet::isFirst() const { return currentRecord == 0; }

gsl::index DataSets::MemoryDataSet::getFirst() const { return 1; }

gsl::index DataSets::MemoryDataSet::getLast() const { return data.size() - 2; }

bool DataSets::MemoryDataSet::isBegin() const { return currentRecord == 0; }

bool DataSets::MemoryDataSet::isEnd() const { return currentRecord == static_cast<gsl::index>(data.size() - 1); }
gsl::index DataSets::MemoryDataSet::getCurrentRecord() const { return getLast(); }

void DataSets::MemoryDataSet::resetBegin() { currentRecord = 0; }
void DataSets::MemoryDataSet::resetEnd() { currentRecord = getLast() + 1; }

DataSets::MemoryDataSet::iterator DataSets::MemoryDataSet::begin() { return iterator(this, 1); }

DataSets::MemoryDataSet::iterator DataSets::MemoryDataSet::end() { return iterator(this, getLast() + 1); }

DataContainer &DataSets::MemoryDataSet::getCell(gsl::index row, gsl::index column) { return (*data[row])[column]; }

std::shared_ptr<DataSets::MemoryViewDataSet> DataSets::MemoryDataSet::fullView() {
  std::vector<std::string> fieldNames;
  std::vector<ValueType> fieldTypes;
  std::vector<std::pair<int, int>> fieldIndices;
  auto fields1 = getFields();
  std::for_each(fields1.begin(), fields1.end(),
                [&fieldNames, &fieldTypes, &fieldIndices](const DataSets::BaseField *field) {
                  fieldNames.emplace_back(field->getName());
                  fieldTypes.emplace_back(field->getFieldType());
                  fieldIndices.emplace_back(0, field->getIndex());
                });
  auto result = std::make_shared<DataSets::MemoryViewDataSet>(getName(), fieldNames, fieldTypes, fieldIndices);
  result->rawData()->emplace_back();
  for (auto val : *this) {
    result->rawData()->emplace_back(std::vector<DataSetRow *>{val});
  }
  result->rawData()->emplace_back();
  result->resetBegin();
  return result;
}
void DataSets::MemoryDataSet::setCurrentRecord(gsl::index pos) {
  currentRecord = pos;
}

#pragma clang diagnostic pop
