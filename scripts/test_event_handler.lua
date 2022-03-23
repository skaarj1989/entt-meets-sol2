function test_event_handler(e)
    print('[lua:test_event_handler] received TestEvent: ' .. tostring(e))
end

-- global variable = long-lived connection
conn = dispatcher:connect(TestEvent, test_event_handler)
