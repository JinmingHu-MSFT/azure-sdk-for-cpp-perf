#include "track2_test.h"

#include <atomic>
#include <cassert>
#include <thread>

#include "blobs/blob.hpp"

#include "credential.h"

int track2_test_download(int64_t blobSize, int numBlobs, int concurrency)
{
  using namespace Azure::Storage;
  using namespace Azure::Storage::Blobs;

  auto cred = std::make_shared<SharedKeyCredential>(accountName, accountKey);

  std::string blobName = blobNamePrefix + std::to_string(blobSize);

  auto client = BlockBlobClient(
      std::string("https://") + accountName + ".blob.core.windows.net/" + containerName + "/"
          + blobName,
      cred);

  std::string blobContent;
  blobContent.resize(blobSize);
  FillBuffer(&blobContent[0], blobContent.size());

  try
  {
    auto getPropertiesResult = client.GetProperties();
    if (getPropertiesResult->ContentLength != blobSize)
    {
      throw std::runtime_error("");
    }
  }
  catch (std::runtime_error&)
  {
    client.UploadFromBuffer(
        reinterpret_cast<const uint8_t*>(blobContent.data()), blobContent.length());
  }

  std::atomic<int> counter(numBlobs);
  auto threadFunc = [&]() {
    while (true)
    {
      int i = counter.fetch_sub(1);
      if (i <= 0)
      {
        break;
      }
      DownloadBlobToBufferOptions options;
      options.InitialChunkSize = blobSize;
      options.ChunkSize = blobSize;
      options.Concurrency = 1;
      client.DownloadToBuffer(
          reinterpret_cast<uint8_t*>(&blobContent[0]), blobContent.size(), options);
    }
  };

  auto start = std::chrono::steady_clock::now();
  std::vector<std::thread> ths;
  for (int i = 0; i < concurrency; ++i)
  {
    ths.emplace_back(threadFunc);
  }
  for (auto& th : ths)
  {
    th.join();
  }
  auto end = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

int track2_test_upload(int64_t blobSize, int numBlobs, int concurrency)
{
  using namespace Azure::Storage;
  using namespace Azure::Storage::Blobs;

  auto cred = std::make_shared<SharedKeyCredential>(accountName, accountKey);

  std::string blobName = blobNamePrefix + std::to_string(blobSize);

  std::string blobContent;
  blobContent.resize(blobSize);
  FillBuffer(&blobContent[0], blobContent.size());

  std::atomic<int> counter(numBlobs);
  auto threadFunc = [&]() {
    while (true)
    {
      int i = counter.fetch_sub(1);
      if (i <= 0)
      {
        break;
      }
      auto client = BlockBlobClient(
          std::string("https://") + accountName + ".blob.core.windows.net/" + containerName + "/"
              + blobName + "-" + std::to_string(i),
          cred);

      UploadBlobOptions options;
      options.ChunkSize = blobSize;
      options.Concurrency = 1;
      client.UploadFromBuffer(
          reinterpret_cast<const uint8_t*>(blobContent.data()), blobContent.size(), options);
    }
  };

  auto start = std::chrono::steady_clock::now();
  std::vector<std::thread> ths;
  for (int i = 0; i < concurrency; ++i)
  {
    ths.emplace_back(threadFunc);
  }
  for (auto& th : ths)
  {
    th.join();
  }
  auto end = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}