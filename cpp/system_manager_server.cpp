#include <iostream>
#include <memory>
#include <string>
#include <queue>
#include <map>
#include <condition_variable>
#include <mutex>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "systemmanagerservice.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;

namespace grpcexamples {

std::ostream& operator<<(std::ostream& os, PowerState state) {
    switch (state) {
        case POWERSTATE__OFF:
            os << "POWERSTATE__OFF";
            break;
        case POWERSTATE__TRANSITION_TO_LPM:
            os << "POWERSTATE__TRANSITION_TO_LPM";
            break;
        case POWERSTATE__LPM:
            os << "POWERSTATE__LPM";
            break;
        case POWERSTATE__TRANSITION_TO_ACTIVE:
            os << "POWERSTATE__TRANSITION_TO_ACTIVE";
            break;
        case POWERSTATE__ACTIVE:
            os << "POWERSTATE__ACTIVE";
            break;
    }
    return os;
}

class SystemManagerServerImpl final : public SystemManagerService::Service {
public:

    grpc::Status NotifyOnPowerStateChange(grpc::ServerContext* context,
                                          NotifyOnPowerStateChangeRequest const* request,
                                          grpc::ServerWriter< PowerStateChangeEvent >* writer) override {
        std::cout << "Client " << request->client_id() << " has subsribed for power state change\n";

        PowerStateChangeEvent event{};

        std::unique_lock< std::mutex > lock(m_mutex);
        while(m_queue.empty()) {
            m_conv.wait(lock);
        }
        lock.unlock();
        const auto state = m_queue.front(); m_queue.pop();
        std::cout << "Send state " << state << "\n";
        event.set_new_state(state);
        writer->Write(event);

        return grpc::Status::OK;
    }

    grpc::Status AcknowledgePowerStateChange(grpc::ServerContext* context,
                                             AcknowledgePowerStateChangeRequest const* request,
                                             NoResponseRequired* response) override {
        std::cout << "Received acknowledge from " << request->client_id() << " for state " << request->state() << "\n";

        if (m_powerState != nextState[m_powerState]) {
            queueNextPowerState();
        }
        return grpc::Status::OK;
    }

    grpc::Status NotifyOnKeepActive(::grpc::ServerContext* context,
                                    grpc::ServerReader< KeepActiveRequest >* reader,
                                    NoResponseRequired* response) override {
        
        std::cout << "Enter notify keep active\n";
        KeepActiveRequest request{};
        while (reader->Read(&request)) {
            std::cout << "Keep active received from client " << request.client_id() << "\n";
            if (POWERSTATE__OFF == m_powerState) {
                queueNextPowerState();
            }
        }

        std::cout << "Exit notify keep active\n";
        return grpc::Status::OK;
    }

private:
    void queueNextPowerState() {
        std::cout << "Current state " << m_powerState << ". Next state " << nextState[m_powerState] << "\n";
        std::unique_lock< std::mutex > lock(m_mutex);
        m_powerState = nextState[m_powerState];
        m_queue.push(m_powerState);
        lock.unlock();
        m_conv.notify_all();
    }

    grpcexamples::PowerState m_powerState{POWERSTATE__OFF};
    std::queue< grpcexamples::PowerState > m_queue{};
    std::map< grpcexamples::PowerState, grpcexamples::PowerState> nextState = {
        { POWERSTATE__OFF, POWERSTATE__TRANSITION_TO_LPM },
        { POWERSTATE__TRANSITION_TO_LPM, POWERSTATE__LPM },
        { POWERSTATE__LPM, POWERSTATE__TRANSITION_TO_ACTIVE },
        { POWERSTATE__TRANSITION_TO_ACTIVE, POWERSTATE__ACTIVE },
        { POWERSTATE__ACTIVE, POWERSTATE__ACTIVE }
    };
    std::condition_variable m_conv{};
    std::mutex m_mutex{};
};
} //namespace grpcexamples

int main(int argc, char** argv) {
    std::string server_address("0.0.0.0:50051");
    grpcexamples::SystemManagerServerImpl service{};

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}
