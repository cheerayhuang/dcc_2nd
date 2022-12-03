/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: main.cc
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 11-21-2022 16:36:34
 * @version $Revision$
 *
 **************************************************************************/

#pragma once

#include "RandomUtil.h"
#include "ColValue.h"

#include <arrow/api.h>
#include <arrow/compute/api.h>
#include <arrow/scalar.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/record_batch.h>
#include <parquet/exception.h>


#include <chrono>
#include <iostream>
#include <map>

class ArrowUtil {

    public:

        static void createDataFile(long total, const std::string& filePath) {
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

            auto strings1 = std::make_unique<std::string[]>(total);
            auto ints = std::make_unique<int[]>(total);
            auto doubles = std::make_unique<double[]>(total);
            auto strings2 = std::make_unique<std::string[]>(total);


            for (long i = 0; i < total; ++i) {
                strings1[i] = RandomUtil::randomString(20);
                ints[i] = RandomUtil::randomInt(1, 10);
                doubles[i] = RandomUtil::randomDouble(0, 1000000000);
                strings2[i] = RandomUtil::randomString(2);
            }

            auto colValue = std::make_unique<ColValue>();
            colValue->total = total;
            colValue->stringVal1 = std::move(strings1);
            colValue->intVal = std::move(ints);
            colValue->doubles = std::move(doubles);
            colValue->stringVal2 = std::move(strings2);

            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            std::cout << "the duration of preparing datas: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()/1000 << "ms" << std::endl;
            writeDataToFile(*colValue, filePath);
        }


        static void writeDataToFile(const ColValue& colValue, const std::string& filePath) {
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

            std::shared_ptr<arrow::Schema> schema = arrow::schema ({
                arrow::field("s1", arrow::large_utf8()),
                arrow::field("i1", arrow::int32()),
                arrow::field("d1", arrow::float64()),
                arrow::field("s2", arrow::large_utf8()),
            });

            arrow::LargeStringBuilder stringBuilder1;
            arrow::Int32Builder intBuilder;
            arrow::DoubleBuilder doubleBuilder;
            arrow::LargeStringBuilder stringBuilder2;

            for (long i = 0; i < colValue.total; ++i) {
                auto s = stringBuilder1.Append(colValue.stringVal1[i]);
                s = intBuilder.Append(colValue.intVal[i]);
                s = doubleBuilder.Append(colValue.doubles[i]);
                s = stringBuilder2.Append(colValue.stringVal2[i]);
            }

            std::shared_ptr<arrow::Array> stringArray1;
            std::shared_ptr<arrow::Array> intArray;
            std::shared_ptr<arrow::Array> doubleArray;
            std::shared_ptr<arrow::Array> stringArray2;
            auto s = stringBuilder1.Finish(&stringArray1);
            s = intBuilder.Finish(&intArray);
            s = doubleBuilder.Finish(&doubleArray);
            s = stringBuilder2.Finish(&stringArray2);

            std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, {
                stringArray1,
                intArray,
                doubleArray,
                stringArray2
            });

            std::shared_ptr<arrow::io::FileOutputStream> outfile;
            PARQUET_ASSIGN_OR_THROW(outfile, arrow::io::FileOutputStream::Open(filePath));

            std::shared_ptr<arrow::ipc::RecordBatchWriter> writer;
            PARQUET_ASSIGN_OR_THROW(writer, arrow::ipc::MakeFileWriter(outfile.get(), schema, arrow::ipc::IpcWriteOptions::Defaults()));

            PARQUET_THROW_NOT_OK(writer->WriteTable(*table));
            PARQUET_THROW_NOT_OK(writer->Close());
            PARQUET_THROW_NOT_OK(outfile->Close());

            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            std::cout << "the duration of writing datas: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()/1000 << "ms" << std::endl;
        }

        static std::shared_ptr<ColValue> readDataFromFile(const std::string& filePath, long total) {
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

            std::shared_ptr<ColValue> colValue = std::make_shared<ColValue>();
            colValue->total = total;
            colValue->stringVal1 = std::move(std::make_unique<std::string[]>(total));
            colValue->intVal = std::move(std::make_unique<int[]>(total));
            colValue->doubles = std::move(std::make_unique<double[]>(total));
            colValue->stringVal2 = std::move(std::make_unique<std::string[]>(total));

            std::shared_ptr<arrow::io::ReadableFile> infile;
            PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(filePath));

            std::shared_ptr<arrow::ipc::RecordBatchFileReader> batchReader;
            PARQUET_ASSIGN_OR_THROW(batchReader, arrow::ipc::RecordBatchFileReader::Open(infile, arrow::ipc::IpcReadOptions::Defaults()));

            int recordBatchCount = batchReader->num_record_batches();
            long data_offset = 0;

            for (int i = 0; i < recordBatchCount; ++i) {
                PARQUET_ASSIGN_OR_THROW(auto batch, batchReader->ReadRecordBatch(i));
                int64_t count = batch->num_rows();
                std::shared_ptr<arrow::LargeStringArray> stringArray1 = std::static_pointer_cast<arrow::LargeStringArray>(batch->GetColumnByName("s1"));
                for (int j = 0; j < count; ++j) {
                    colValue->stringVal1[data_offset + j] = stringArray1->GetString(j);
                }

                std::shared_ptr<arrow::Int32Array> intArray = std::static_pointer_cast<arrow::Int32Array>(batch->GetColumnByName("i1"));
                for (int j = 0; j < count; ++j) {
                    colValue->intVal[data_offset + j] = intArray->GetView(j);
                }

                std::shared_ptr<arrow::DoubleArray> doubleArray = std::static_pointer_cast<arrow::DoubleArray>(batch->GetColumnByName("d1"));
                for (int j = 0; j < count; ++j) {
                    colValue->doubles[data_offset + j] = doubleArray->GetView(j);
                }

                std::shared_ptr<arrow::LargeStringArray> stringArray2 = std::static_pointer_cast<arrow::LargeStringArray>(batch->GetColumnByName("s2"));
                for (int j = 0; j < count; ++j) {
                    colValue->stringVal2[data_offset + j] = stringArray2->GetString(j);
                }
                data_offset += count;
            }

            PARQUET_THROW_NOT_OK(infile->Close());


            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            std::cout << "the duration of reading datas: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()/1000 << "ms" << std::endl;

            return colValue;
        }
};


