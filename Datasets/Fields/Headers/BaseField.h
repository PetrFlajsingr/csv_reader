//
// Created by Petr Flajsingr on 27/08/2018.
//

#ifndef DATASETS_FIELDS_HEADERS_BASEFIELD_H_
#define DATASETS_FIELDS_HEADERS_BASEFIELD_H_

#include <string>
#include <utility>
#include <functional>
#include "Types.h"
#include "Exceptions.h"

namespace DataSets {
class BaseDataSet;
struct DataSetRow;

/**
 * Interface pro fields datasetu.
 */
class BaseField {
 protected:
  std::string fieldName;  //< Nazev reprezentovaneho sloupce

  uint64_t index;  //< Index sloupce

 public:
  /**
   *
   * @return Nazev pole
   */
  const std::string &getFieldName() const {
    return fieldName;
  }

 protected:
  friend class BaseDataSet; //< Pro pristup k primemu nastaveni dat

  BaseDataSet *dataSet;  //< Rodicovsky dataset

  /**
   * Nastaveni hodnoty field.
   * @param data pointer na data
   */
  virtual void setValue(void *data) = 0;

  /**
   * Nastaveni dat v datasetu.
   *
   * Tato funkce zpristupnuje setValue potomkum.
   * @param data data pro ulozeni
   * @param type typ dat
   */
  void setData(void *data, ValueType type);

 public:
  /**
   * Nastaveni datasetu, nazvu a indexu field
   * @param fieldName Nazev field
   * @param dataset Rodicovsky dataset
   * @param index Index pole v zaznamu
   */
  explicit BaseField(const std::string &fieldName,
                     BaseDataSet *dataset,
                     uint64_t index) : fieldName(fieldName),
                                       dataSet(dataset),
                                       index(index) {}

  virtual ~BaseField() = default;

  /**
   * Typ dat ulozenych ve Field
   * @return
   */
  virtual ValueType getFieldType() = 0;

  /**
   * Nastaveni hodnoty pole pomoci string
   * @param value
   */
  virtual void setAsString(const std::string &value) = 0;

  /**
   * Navrat hodnoty v poli jako string
   * @return
   */
  virtual std::string getAsString() = 0;

  /**
   *
   * @return Index Field v DataSet
   */
  uint64_t getIndex() {
    return index;
  }

  /**
   * Funkce pro razeni polozek datasetu podle jejich datoveho typu
   * @param order poradi (ascending nebo descending)
   * @return porovnavaci funkce, ktera vraci:
   *    0 pokud se prvky rovnaji
   *    1 pokud je prvni vetsi
   *    -1 pokud je prvni mensi
   */
  virtual std::function<int8_t (DataSetRow *, DataSetRow *)> getCompareFunction() = 0;

};
}  // namespace DataSets

#endif  // DATASETS_FIELDS_HEADERS_BASEFIELD_H_
