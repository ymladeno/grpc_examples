from grpc_tools import protoc

protoc.main((
    '',
    '-I../proto',
    '--python_out=.',
    '--grpc_python_out=.',
    'systemmanagerservice.proto','systemmanagerservice_signals.proto','systemmanagerservice_types.proto'
))
