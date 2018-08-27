//
// Created by Petr Flajsingr on 27/08/2018.
//

#include "IntegerField.h"
#include "../../Misc/Utilities.h"

IntegerField::IntegerField(const std::string &fieldName, IDataset *dataset, unsigned long index) : IField(fieldName,
                                                                                                          dataset,
                                                                                                          index) {}

ValueType IntegerField::getFieldType() {
    return INTEGER_VAL;
}

void IntegerField::setAsString(const std::string &value) {
    this->data = Utilities::StringToInt(value);
    IField::setDatasetData((u_int8_t *) &this->data, getFieldType());
}

std::string IntegerField::getAsString() {
    return std::to_string(this->data);
}

void IntegerField::setValue(u_int8_t *data) {
    std::memcpy(&this->data, data, sizeof(int));
}

void IntegerField::setAsInteger(int value) {
    this->data = value;
    IField::setDatasetData((u_int8_t *) &this->data, getFieldType());
}

int IntegerField::getAsInteger() {
    return this->data;
}

