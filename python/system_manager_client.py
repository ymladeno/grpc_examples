import logging
from system_manager_types import PowerState
import systemmanagerservice_pb2_grpc
import systemmanagerservice_signals_pb2
import systemmanagerservice_types_pb2
from time import sleep
from threading import Thread
import grpc

logger = logging.getLogger(__name__)

class SystemManagerClient:
    def __init__(self):
        self.channel = None
        self.stub = None
        self.running = False
        self.send_thread = Thread(target = self.send_keep_active)
        self.translation_to_grpc = {
            systemmanagerservice_types_pb2.POWERSTATE__ACTIVE: PowerState.ACTIVE,
            systemmanagerservice_types_pb2.POWERSTATE__TRANSITION_TO_ACTIVE: PowerState.TRANSITION_TO_ACTIVE,
            systemmanagerservice_types_pb2.POWERSTATE__LPM: PowerState.LPM,
            systemmanagerservice_types_pb2.POWERSTATE__TRANSITION_TO_LPM: PowerState.TRANSITION_TO_LPM
        }

    def connect(self):
        # self.channel = grpc.insecure_channel('localhost:50051', options=(('grpc.enable_http_proxy', 0),))
        self.channel = grpc.insecure_channel('localhost:50051')
        self.stub = (
            systemmanagerservice_pb2_grpc.SystemManagerServiceStub(
                self.channel
            )
        )
        self.running = True

    def disconnect(self):
        logger.info(f"Disconnect")
        self.channel = None
        self.stub = None
        self.running = False
        self.send_thread.join()

    def wait_power_state(self):
        logger.info(f"+ Wait for power state")
        return self.stub.NotifyOnPowerStateChange(
            systemmanagerservice_signals_pb2.NotifyOnPowerStateChangeRequest(client_id="test")
        )

    def start_sending_keep_active(self):
        self.send_thread.start()

    def send_keep_active(self):
        while self.running:
            logger.info(f"Request keep active")
            self.stub.NotifyOnKeepActive(self._generate_keep_request())
            sleep(1)

    def _generate_keep_request(self):
        yield systemmanagerservice_signals_pb2.KeepActiveRequest(client_id = "test")

    def acknowledge_power_state(self, powerStateEvent):
        logger.info(f"+ Ack power state: {powerStateEvent}")
        self.stub.AcknowledgePowerStateChange(
            systemmanagerservice_signals_pb2.AcknowledgePowerStateChangeRequest(
                client_id = "test", state = powerStateEvent.new_state))
        logger.info(f"+ Exit ack power state")

def run():
    logger.info(f"Enter run")
    client = SystemManagerClient()
    client.connect()

    client.start_sending_keep_active()

    exit_flag = False
    while (True):
        for response in client.wait_power_state():
            logger.info(f"Received power state {response}")
            client.acknowledge_power_state(response)
            if response.new_state == systemmanagerservice_types_pb2.POWERSTATE__ACTIVE:
                exit_flag = True
                break
        if exit_flag:
            break

    logger.info(f"LOGN BUTTON PRESS")
    sleep(5)
    client.disconnect()

if __name__ == '__main__':
    formatter = "[%(asctime)s] %(name)s {%(filename)s:%(lineno)d} %(levelname)s - %(message)s"
    logging.basicConfig(level = logging.INFO, format = formatter)
    run()