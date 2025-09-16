#pragma once
#include "client/MatrixClient.h"
#include <memory>
#include "TextBuffer.h"

namespace App {
    void SetupMessageSending(std::shared_ptr<TextBuffer>& sharedBuffer, MatrixClient& matrix);
}
