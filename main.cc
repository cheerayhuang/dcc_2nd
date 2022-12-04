#include "ArrowUtil.h"
#include "ValidUtil.h"
#include "mmap_reader.h"
#include "mmap_writer.h"
#include "sending_flow.h"
#include "thread_pool.h"

DEFINE_int32(file_rows, 1, "Specifies the total rows of the arrow file.");
DEFINE_int32(jobs, 1, "Specifies the number of jobs (threads) to run simultaneously.");
DEFINE_int32(port, 10049, "Specifies the remote hosts' port.");
DEFINE_int64(segment_size, 0, "Specifies the segment size for transport.");
DEFINE_bool(gen_file, false, "generate test arrow file.");
DEFINE_bool(verify, false, "generate test arrow file.");
DEFINE_bool(compress, false, "Specifies whether the payloads need compressing in transport.");
DEFINE_string(remote, "127.0.0.1", "Specifies the remote hosts' IP.");
DEFINE_string(file_path, "data.arrow", "Specifies  the file path of the arrow file.");


using namespace std::literals;

int main(int argc, char**argv) {
    //std::string createFilePath = argv[1];
    //int createRowCount = std::stoi(argv[2]);

    gflags::SetVersionString("0.1.0");
    gflags::SetUsageMessage("Transport Big Arrow File.");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    spdlog::set_pattern("[send %t] %+ ");
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_every(std::chrono::seconds(2));

    if (FLAGS_gen_file) {
        spdlog::info("begin to gen data file: {}, {}rows.", FLAGS_file_path, FLAGS_file_rows);
        ArrowUtil::createDataFile(FLAGS_file_rows, FLAGS_file_path);
        return 0;
    }

    if (FLAGS_verify) {
        /*
         * read arrow file and verify it.
         */
        //auto colValue = ArrowUtil::readDataFromFile(FLAGS_file_path, FLAGS_file_rows);

        /*
        std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(2);
        for (auto i = 0; i < FLAGS_file_rows; ++i) {
            std::cout << colValue->stringVal1[i]
                << " " << colValue->intVal[i]
                << " " << colValue->doubles[i]
                << " " << colValue->stringVal2[i]
                << std::endl;
        }*/

        /*auto validResult = ValidUtil::getValidResult(
            colValue->stringVal1.get(),
            colValue->intVal.get(),
            colValue->doubles.get(),
            colValue->stringVal2.get(),
            colValue->total
        );*/
        auto validResult = ValidUtil::getValidResult(FLAGS_file_path, FLAGS_file_rows);

        SPDLOG_WARN("{}", validResult);
        return 0;
    }

    // read file
    auto [g_mmap_file_buff, g_file_size] = MMapReader::ReadArrowFile(FLAGS_file_path);
    spdlog::info("file buff ptr: {}, g_file_size: {}", g_mmap_file_buff, g_file_size);


    auto g_thread_pool = std::make_unique<ThreadPool>(FLAGS_jobs+1);
    std::vector<std::future<void>> future_res;

    // count time
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    auto r_supervisor = g_thread_pool->enqueue(SendingFlow::Supervise, start);

    // split file
    auto file_slices = FileSlice::SplitFile(g_mmap_file_buff, g_file_size);
    spdlog::info("split file to {} slices, each slice has {} bytes", FLAGS_jobs, FLAGS_segment_size);


    if (auto r = SendingFlow::Init(g_file_size, file_slices.size()); r != 0) {
        SPDLOG_ERROR("Init sending flow failed.");
        return -1;
    }
    for(auto && f : file_slices) {
        SPDLOG_INFO("slice info: index={}, len={}", f->index(), f->len());
        future_res.push_back( g_thread_pool->enqueue(&SendingFlow::Run, f) );
    }
    SendingFlow::Destory();

    for (auto && r : future_res) {
        SPDLOG_INFO("Waiting future...");
        r.get();
    }
    SPDLOG_INFO("Waiting Supervisor future...");
    r_supervisor.get();
    //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    // --
    // test for zstd compress
    // --
    /*
    auto file_size = std::accumulate(file_slices.begin(), file_slices.end(), 0,
        [](size_t s, std::shared_ptr<FileSlice> f) -> size_t {
            return s + f->c_len();
        }
    );
    auto mmap_file_buf = MMapWriter::MMapOutputArrowFile(FLAGS_file_path+".test.zst"s, file_size);
    for (auto && f : file_slices) {
        auto pos = f->index();
        auto& chunk_buf = f->compress_buf();
        auto beg = chunk_buf.begin();
        beg++;
        for (; beg != chunk_buf.end(); ++beg) {
            SPDLOG_INFO("chunk len: {}", beg->len_);
            MMapWriter::MMapWriting(
                mmap_file_buf,
                beg->c_,
                pos,
                beg->len_,
                file_size,
                MS_SYNC
            );
            pos += beg->len_;
        }
    }
    MMapWriter::UnMapOutputArrowFile(mmap_file_buf, file_size);
    */

    // --
    // test for compressing big file, print slices compressed.
    // --
    //auto index = 0;
    //for (auto && f : file_slices) {
    //    SPDLOG_INFO("slice #{}: index {}, len {}, c_len {}:", index++, f->index(), f->len(), f->c_len());
    //    auto & chunks = f->compress_buf();
    //    auto c_index = 0;
    //    for (auto && c : chunks) {
    //        SPDLOG_INFO("\tchunk #{}: len {}.", c_index++, c.len_);
    //    }
    //}

    return 0;
}
