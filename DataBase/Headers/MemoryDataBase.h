//
// Created by Petr Flajsingr on 2019-02-08.
//

#ifndef PROJECT_MEMORYDATABASE_H
#define PROJECT_MEMORYDATABASE_H

#include <vector>
#include <string>
#include <BaseDataSet.h>
#include <MemoryViewDataSet.h>
#include <Exceptions.h>
#include <QueryCommon.h>
#include <LexicalAnalyser.h>
#include <SyntaxAnalyser.h>
#include <SemanticAnalyser.h>
#include <MemoryDataSet.h>

namespace DataBase {

struct Table {
  std::shared_ptr<DataSets::MemoryDataSet> dataSet;

  explicit Table(const std::shared_ptr<DataSets::MemoryDataSet> &dataSet);
};

struct View {
  std::shared_ptr<DataSets::MemoryViewDataSet> dataSet;

  explicit View(const std::shared_ptr<DataSets::MemoryViewDataSet> &dataSet);
};

class MemoryDataBase {
 public:
  explicit MemoryDataBase(const std::string &name);

  void addTable(std::shared_ptr<DataSets::MemoryDataSet> dataSet);

  void removeTable(std::string_view tableName);

  const Table &tableByName(std::string_view tableName) const;

  std::shared_ptr<View> execSimpleQuery(
      std::string_view query,
      bool keepView,
      std::string_view viewName);

  std::shared_ptr<DataSets::BaseDataSet> execAggregateQuery(
      std::string_view query,
      std::string_view viewName);

  std::string_view getName() const;

  void validateQuery(StructuredQuery query) const;

 private:
  std::vector<Table> tables;
  std::vector<std::shared_ptr<View>> views;

  StructuredQuery parseQuery(std::string_view query);

  std::string name;

  LexicalAnalyser lexicalAnalyser;
  SyntaxAnalyser syntaxAnalyser;
  SemanticAnalyser semanticAnalyser;

  // operations on views
  std::shared_ptr<DataSets::MemoryViewDataSet> doJoin(const StructuredQuery &query);
  std::shared_ptr<DataSets::MemoryViewDataSet> doWhere(const StructuredQuery &query,
                                                       std::shared_ptr<DataSets::MemoryViewDataSet> &view);
  std::shared_ptr<DataSets::MemoryViewDataSet> doOrder(const StructuredQuery &query,
                                                       std::shared_ptr<DataSets::MemoryViewDataSet> &view);
  std::shared_ptr<DataSets::MemoryViewDataSet> doProject(const StructuredQuery &query,
                                                         std::shared_ptr<
                                                             DataSets::MemoryViewDataSet> &view);

  // operations on datasets (aggregation needs to create new table)
  std::shared_ptr<DataSets::MemoryDataSet> doAggregation(const StructuredQuery &query,
                                                         std::shared_ptr<
                                                             DataSets::MemoryDataSet> &table);
  std::shared_ptr<DataSets::MemoryDataSet> doHaving(const StructuredQuery &query,
                                                    std::shared_ptr<DataSets::MemoryDataSet> &table);
  std::shared_ptr<DataSets::MemoryDataSet> doOrder(const StructuredQuery &query,
                                                   std::shared_ptr<DataSets::MemoryDataSet> &table);
  std::shared_ptr<DataSets::MemoryDataSet> doProject(const StructuredQuery &query,
                                                     std::shared_ptr<DataSets::MemoryDataSet> &table);
};

}  // namespace DataBase

#endif //PROJECT_MEMORYDATABASE_H
