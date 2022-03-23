listener = {
    foo = 2
}
function listener.receive(evt)
    print('[lua:more_listeners] listener:receive(): TestEvent: ' .. tostring(evt))
end
function listener.method(evt)
    print('[lua:more_listeners] listener:method(): TestEvent: ' .. tostring(evt))
end

listener.connection = dispatcher:connect(TestEvent, listener.receive)
-- connection for 'listener.method' is discarded for purpose
-- listener will be disconnected during garbage collection step
dispatcher:connect(TestEvent, listener.method)
