from concurrent import futures
from queue import Queue
import logging

import grpc

from system_manager_types import PowerState
import systemmanagerservice_pb2_grpc
import systemmanagerservice_signals_pb2
import systemmanagerservice_types_pb2

class SystemManagerServicer(systemmanagerservice_pb2_grpc.SystemManagerServiceServicer):
    def __init__(self):
        self.queue = Queue()
        self.state = PowerState.LPM

        self.convert_power_state = {
            PowerState.ACTIVE: systemmanagerservice_types_pb2.POWERSTATE__ACTIVE,
            PowerState.TRANSITION_TO_ACTIVE: systemmanagerservice_types_pb2.POWERSTATE__TRANSITION_TO_ACTIVE,
            PowerState.LPM: systemmanagerservice_types_pb2.POWERSTATE__LPM,
            PowerState.TRANSITION_TO_LPM: systemmanagerservice_types_pb2.POWERSTATE__TRANSITION_TO_LPM,
        }

        self.next_power_state = {
            PowerState.LPM: PowerState.TRANSITION_TO_ACTIVE,
            PowerState.TRANSITION_TO_ACTIVE: PowerState.ACTIVE,
            PowerState.ACTIVE: PowerState.ACTIVE,
        }

    def NotifyOnPowerStateChange(self, request, context):
        print(f" + Client {request.client_id} has subsribed for power state change")
        new_state = self.queue.get(block = True)
        grpc_state = self.convert_power_state[new_state]
        yield systemmanagerservice_signals_pb2.PowerStateChangeEvent(new_state = grpc_state)

    def AcknowledgePowerStateChange(self, request, context):
        print(f"Client {request.client_id} acknoledge power state {request.state}")

        # new_state = self.next_power_state[request.state]
        # if new_state != self.state:
        #     self.state = new_state
        #     self.queue.put(self.state)
        #     print(f"New state is {self.state}")

        return systemmanagerservice_signals_pb2.NoResponseRequired()

    def NotifyOnKeepActive(self, request_iterator, context):
        for req in request_iterator:
            print(f"Keep active received from client {req.client_id}")

        print(f"Current state is {self.state}")
        new_state = self.next_power_state[self.state]
        if new_state != self.state:
            self.state = new_state
            self.queue.put(self.state)
            print(f"New state is {self.state}")
        return systemmanagerservice_signals_pb2.NoResponseRequired()

def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    systemmanagerservice_pb2_grpc.add_SystemManagerServiceServicer_to_server(
        SystemManagerServicer(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    server.wait_for_termination()

if __name__ == '__main__':
    logging.basicConfig()
    serve()