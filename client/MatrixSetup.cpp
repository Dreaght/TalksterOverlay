#include "MatrixSetup.h"
#include "../Utils.h"

namespace App {
    void SetupMatrix(MatrixClient& matrix, ChatWindow& chat) {
        matrix.SetOnMessage([&](const std::string& roomId, const std::string& msg) {
            chat.OnExternalMessage(ToWString(msg), false);
        });
        matrix.Start();
    }
}
