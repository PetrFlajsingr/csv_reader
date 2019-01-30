//
// Created by Petr Flajsingr on 24/08/2018.
//

#ifndef DATAPROVIDERS_HEADERS_BASEDATAPROVIDER_H_
#define DATAPROVIDERS_HEADERS_BASEDATAPROVIDER_H_

#include <string>
#include <vector>

namespace DataProviders {

/**
 * Jednoduche rozhrani pro cteni a pohyb v zaznamech.
 *
 * Pouziti:
 *  auto provider = new DataProviders::CsvReader(...);
 *  while (!provider->eof()) {
 *      // zpracuj data...
 *      provider->next();
 *  }
 *
 * Pouziti s iteratorem:
 *  auto provider = new DataProviders::CsvReader(...);
 *  for (const auto &row : provider) {
 *      // zpracuj data...
 *  }
 */
class BaseDataProvider {
 public:
  /**
   * Iterator pro velmi snadnou iteraci zaznamy za pouziti for each konstrukce.
   */
  class iterator : public std::iterator<std::input_iterator_tag,
                                        std::vector<std::string>> {
   private:
    BaseDataProvider *provider;

   public:
    explicit iterator(BaseDataProvider *provider)
        : provider(provider) {}

    iterator(const iterator &other) {
      provider = other.provider;
    }

    /**
     * Posun na dalsi zaznam.
     * @return
     */
    iterator &operator++() {
      provider->next();
      return *this;
    }

    /**
     * Posun na dalsi zaznam
     * @return
     */
    const iterator operator++(int) {
      iterator result = *this;
      ++(*this);
      return result;
    }

    /**
     * Zneuziti porovnani begin() == end() pro kontrolu eof()
     * @return
     */
    bool operator==(const iterator &) const {
      return provider->eof();
    }

    /**
     * Zneuziti porovnani begin() == end() pro kontrolu eof()
     * @return
     */
    bool operator!=(const iterator &other) const {
      return !(*this == other);
    }

    /**
     * Dereference na aktualni zaznam
     * @return
     */
    std::vector<std::string> operator*() const {
      return provider->getRow();
    }
  };

  /**
   * Zaznam rozdeleny na sloupce
   * @return
   */
  virtual const std::vector<std::string> &getRow() const = 0;

  /**
   * Hodnota ve vybranem sloupci
   * @param columnIndex Index sloupce
   * @return
   */
  virtual std::string getColumnName(unsigned int columnIndex) const = 0;

  /**
   * Pocet sloupcu zaznamu
   * @return
   */
  virtual uint64_t getColumnCount() const = 0;

  /**
   * Nazvy sloupcu
   * @return
   */
  virtual const std::vector<std::string> &getHeader() const = 0;

  /**
   * Pocet prozatim prectenych zaznamu
   * @return
   */
  virtual uint64_t getCurrentRecordNumber() const = 0;

  /**
   * Presun na nasledujici zaznam
   * @return
   */
  virtual bool next() = 0;

  /**
   * Presun na prvni zaznam
   */
  virtual void first() = 0;

  /**
   * Kontrola dostupnosti zaznamu
   * @return false pokud neni dostupny dalsi zaznam, jinak true
   */
  inline virtual bool eof() const = 0;

  iterator begin() {
    return iterator(this);
  }

  iterator end() {
    return iterator(this);
  }

  virtual ~BaseDataProvider() = default;
};

}  // namespace DataProviders

#endif //  DATAPROVIDERS_HEADERS_BASEDATAPROVIDER_H_
