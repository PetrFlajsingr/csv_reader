//
// Created by Petr Flajsingr on 2019-02-08.
//

#include <utility>
#include <map>
#include <MemoryDataBase.h>
#include <JoinMaker.h>

DataBase::Table::Table(const std::shared_ptr<DataSets::MemoryDataSet> &dataSet)
    : dataSet(dataSet) {}
DataBase::View::View(const std::shared_ptr<DataSets::MemoryViewDataSet> &dataSet)
    : dataSet(dataSet) {}

DataBase::MemoryDataBase::MemoryDataBase(const std::string &name)
    : name(name) {}

std::string_view DataBase::MemoryDataBase::getName() const {
  return name;
}

void DataBase::MemoryDataBase::addTable(std::shared_ptr<DataSets::MemoryDataSet> dataSet) {
  tables.emplace_back(std::make_shared<Table>(dataSet));
}

void DataBase::MemoryDataBase::removeTable(std::string_view tableName) {
  const auto fncFindByName = [tableName]
      (const std::shared_ptr<Table> &table) {
    return table->dataSet->getName() == tableName;
  };

  if (auto it = std::find_if(tables.begin(),
                             tables.end(),
                             fncFindByName);
      it != tables.end()) {
    tables.erase(it);
  } else {
    throw DataBaseException("Table " + std::string(tableName) + " not found.");
  }
}

std::shared_ptr<DataBase::View> DataBase::MemoryDataBase::execSimpleQuery(
    std::string_view query,
    bool keepView,
    std::string_view viewName) {
  auto structQuery = parseQuery(query);

  if (!structQuery.agr.data.empty()) {
    throw DataBaseException("Provided query is not \"simple\", "
                            "use DataBase::MemoryDataBase::execAggregateQuery "
                            "for queries using aggregation");
  }

  std::shared_ptr<View> result;
  if (!structQuery.joins.data.empty()) {
    result = doJoin(structQuery);
  }

  if (!structQuery.where.data.empty()) {
    result = doWhere(structQuery, result->dataSet);
  }

  return result;
}

std::shared_ptr<DataSets::BaseDataSet> DataBase::MemoryDataBase::execAggregateQuery(
    std::string_view query,
    std::string_view viewName) {
  auto structQuery = parseQuery(query);

  return nullptr;
}

void DataBase::MemoryDataBase::validateQuery(const StructuredQuery &query) const {
  std::vector<std::pair<std::string, bool>> tables;
  tables.emplace_back(query.mainTable, false);

  std::vector<std::string> savedTables;
  std::transform(this->tables.begin(),
                 this->tables.end(),
                 std::back_inserter(savedTables),
                 [](const std::shared_ptr<Table> &table) {
                   return table->dataSet->getName();
                 });

  for (auto &val : tables) {
    val.second = std::find(savedTables.begin(), savedTables.end(), val.first)
        != savedTables.end();
  }

  for (const auto &val : query.joins.data) {
    tables.emplace_back(val.joinedTable, false);
  }
  std::string errMsg;
  for (auto &val : tables) {
    val.second = std::find(savedTables.begin(), savedTables.end(), val.first)
        != savedTables.end();
    if (!val.second) {
      errMsg += "Unknown table: " + val.first + "\n";
    }
  }
  if (!errMsg.empty()) {
    throw DataBaseQueryException("DataBase exception: " + errMsg);
  }

  std::vector<std::pair<FieldId, bool>> fields;
  for (const auto &val : query.project.data) {
    fields.emplace_back(val, false);
  }

  for (const auto &val : query.joins.data) {
    fields.emplace_back(val.firstField, false);
    fields.emplace_back(val.secondField, false);
  }
  for (const auto &val : query.having.data) {
    fields.emplace_back(val.first.agreItem.field, false);
  }
  for (const auto &val : query.agr.data) {
    fields.emplace_back(val.field, false);
  }
  for (const auto &val : query.order.data) {
    fields.emplace_back(val.field, false);
  }
  for (const auto &val : query.where.data) {
    fields.emplace_back(val.first.field, false);
  }

  std::map<std::string, std::vector<std::string>> savedFields;
  for (const auto &table : this->tables) {
    savedFields[table->dataSet->getName()] = table->dataSet->getFieldNames();
  }

  for (auto &val : fields) {
    val.second = std::find(savedFields[val.first.table].begin(),
                           savedFields[val.first.table].end(), val.first.column)
        != savedFields[val.first.table].end();
    if (!val.second && !val.first.column.empty()) {
      errMsg +=
          "Unknown field: " + val.first.column + " in table: " + val.first.table
              + "\n";
    }
  }
  if (!errMsg.empty()) {
    throw DataBaseQueryException("DataBase exception: " + errMsg);
  }

  for (const auto &val : query.joins.data) {
    auto field1Type =
        tableByName(val.firstField.table)->dataSet->fieldByName(val.firstField.column)->getFieldType();
    auto field2Type =
        tableByName(val.secondField.table)->dataSet->fieldByName(val.secondField.column)->getFieldType();
    if (field1Type != field2Type) {
      errMsg = "Field " + val.firstField.table + "." + val.firstField.column
          + " has different data type than field" + val.secondField.table + "."
          + val.secondField.column;
      throw DataBaseQueryException("DataBase exception: " + errMsg);
    }
  }
}

DataBase::StructuredQuery DataBase::MemoryDataBase::parseQuery(std::string_view query) {
  lexicalAnalyser.setInput(std::string(query));
  syntaxAnalyser.setInput(lexicalAnalyser.getAllTokens());
  auto strQuery = syntaxAnalyser.analyse();
  semanticAnalyser.setInput(strQuery);
  auto semQuery = semanticAnalyser.analyse();
  validateQuery(semQuery);
  return semQuery;
}

std::shared_ptr<DataBase::Table> DataBase::MemoryDataBase::tableByName(std::string_view tableName) const {
  for (auto &table : tables) {
    if (table->dataSet->getName() == tableName) {
      return table;
    }
  }
  std::string errMsg = "Table named \"" + std::string(name)
      + "\" not found. DataBase::MemoryDataBase::tableByName";
  throw InvalidArgumentException(errMsg.c_str());
}

std::shared_ptr<DataBase::View> DataBase::MemoryDataBase::doJoin(
    const DataBase::StructuredQuery &query) {
  auto joinItem1 = query.joins.data[0];
  auto table1 = tableByName(joinItem1.firstField.table);
  auto table2 = tableByName(joinItem1.secondField.table);
  JoinMaker joinMaker(table1, joinItem1.firstField.column,
                      table2, joinItem1.secondField.column);
  auto joinResult = joinMaker.join(joinItem1.type);

  for (gsl::index i = 1; i < query.joins.data.size(); ++i) {
    auto joinItem = query.joins.data[i];
    JoinMaker maker(joinResult,
                    joinItem.firstField.column,
                    tableByName(joinItem.secondField.table),
                    joinItem.secondField.column);
    joinResult = maker.join(query.joins.data[i].type);
  }

  return joinResult;
}

std::shared_ptr<DataBase::View> DataBase::MemoryDataBase::doWhere(
    const DataBase::StructuredQuery &query,
    std::shared_ptr<DataSets::MemoryViewDataSet> &view) {
  DataSets::FilterOptions filterOptions;
  for (const auto &whereItem : query.where.data) {
    auto field = view->fieldByName(whereItem.first.field.column);
    std::vector<std::string> values;
    std::transform(whereItem.first.constValues.begin(),
                   whereItem.first.constValues.end(),
                   std::back_inserter(values),
                   [](const std::pair<ConstType, std::string> &value) {
                     if (value.first == ConstType::string) {
                       return value.second.substr(1, value.second.size() - 2);
                     } else {
                       return value.second;
                     }
                   });
    filterOptions.addOption(field, values,
                            DataSets::condOpToFilterOp(whereItem.first.condOperator));
  }
  return std::make_shared<View>(std::dynamic_pointer_cast<DataSets::MemoryViewDataSet>(
      view->filter(filterOptions)));
}

std::shared_ptr<DataSets::MemoryViewDataSet> DataBase::MemoryDataBase::doOrder(
    const DataBase::StructuredQuery &query,
    std::shared_ptr<DataSets::MemoryViewDataSet> &view) {
  DataSets::SortOptions sortOptions;
  for (const auto &option : query.order.data) {
    sortOptions.addOption(view->fieldByName(option.field.column),
                          option.order == Order::asc ? SortOrder::Ascending
                                                     : SortOrder::Descending);
  }
  view->sort(sortOptions);
  return view;
}

std::shared_ptr<DataSets::MemoryViewDataSet> DataBase::MemoryDataBase::doProject(
    const DataBase::StructuredQuery &query,
    std::shared_ptr<
        DataSets::MemoryViewDataSet> &view) {
  throw NotImplementedException();
}

std::shared_ptr<DataSets::MemoryDataSet> DataBase::MemoryDataBase::doAggregation(
    const DataBase::StructuredQuery &query,
    std::shared_ptr<
        DataSets::MemoryDataSet> &table) {
  throw NotImplementedException();
}

std::shared_ptr<DataSets::MemoryDataSet> DataBase::MemoryDataBase::doHaving(
    const DataBase::StructuredQuery &query,
    std::shared_ptr<DataSets::MemoryDataSet> &table) {
  throw NotImplementedException();
}

std::shared_ptr<DataSets::MemoryDataSet> DataBase::MemoryDataBase::doOrder(const DataBase::StructuredQuery &query,
                                                                           std::shared_ptr<
                                                                               DataSets::MemoryDataSet> &table) {
  DataSets::SortOptions sortOptions;
  for (const auto &option : query.order.data) {
    sortOptions.addOption(table->fieldByName(option.field.column),
                          option.order == Order::asc ? SortOrder::Ascending
                                                     : SortOrder::Descending);
  }
  table->sort(sortOptions);
  return table;
}

std::shared_ptr<DataSets::MemoryDataSet> DataBase::MemoryDataBase::doProject(
    const DataBase::StructuredQuery &query,
    std::shared_ptr<DataSets::MemoryDataSet> &table) {
  throw NotImplementedException();
}
