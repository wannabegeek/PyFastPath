#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include <fastpath/fastpath.h>
#include <fastpath/transport/TCPTransport.h>
#include <fastpath/event/DataEvent.h>
#include <fastpath/event/TimerEvent.h>
#include <fastpath/event/SignalEvent.h>

namespace py = pybind11;

class PyEvent : public fp::Event {
public:
    using fp::Event::Event;

    const bool __notify(const fp::EventType &eventType) noexcept override { return false; }
    void __destroy() noexcept override {}
    const bool isEqual(const fp::Event &other) const noexcept { return false; }
};

class PyQueue : public fp::Queue {
private:
    const bool __enqueue(queue_value_type &&event) noexcept override {
        return false;
    }
public:
    using fp::Queue::Queue;

    const fp::status dispatch() noexcept override {
        PYBIND11_OVERLOAD_PURE(fp::status, fp::Queue, dispatch, );
    }

    const fp::status dispatch(const std::chrono::milliseconds &timeout) noexcept override {
        PYBIND11_OVERLOAD_PURE(fp::status, fp::Queue, dispatch, timeout);
    }

    const fp::status try_dispatch() noexcept override {
        PYBIND11_OVERLOAD_PURE(fp::status, fp::Queue, try_dispatch, );
    }

    const size_t eventsInQueue() const noexcept override {
        PYBIND11_OVERLOAD_PURE(size_t, fp::Queue, eventsInQueue, );
    }
};

class PyTransport : public fp::Transport {
protected:
    std::unique_ptr<fp::TransportIOEvent> createReceiverEvent(const std::function<void(const fp::Transport *, fp::Transport::MessageType &)> &messageCallback) {
        return nullptr;
    }
public:
    using fp::Transport::Transport;
    fp::status sendMessage(const fp::Message &message) noexcept override {
        PYBIND11_OVERLOAD_PURE(fp::status, fp::Transport, sendMessage, message);
    }

    fp::status sendMessageWithResponse(const fp::Message &message, fp::Message &reply, std::chrono::duration<std::chrono::milliseconds> &timeout) noexcept override {
//        PYBIND11_OVERLOAD_PURE(fp::status, fp::Transport, sendMessage, message, reply, );
        return fp::OK;
    }

    fp::status sendReply(const fp::Message &reply, const fp::Message &request) noexcept override {
        PYBIND11_OVERLOAD_PURE(fp::status, fp::Transport, sendMessage, reply, request);
    }

    const bool valid() const noexcept override {
        PYBIND11_OVERLOAD_PURE(bool, fp::Transport, valid, );
    }
};

PYBIND11_PLUGIN(fastpath) {
    py::module m("fastpath", R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: pbtest

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc");

    py::enum_<fp::status>(m, "status")
        .value("OK", fp::OK)
        .value("EVM_NOTRUNNING", fp::EVM_NOTRUNNING)
        .value("NO_EVENTS", fp::NO_EVENTS)
        .value("INTERNAL_ERROR", fp::INTERNAL_ERROR)
        .value("CANNOT_SEND", fp::CANNOT_SEND)
        .value("ALREADY_ACTIVE", fp::ALREADY_ACTIVE)
        .value("NOT_ACTIVE", fp::NOT_ACTIVE)
        .value("CANNOT_CREATE", fp::CANNOT_CREATE)
        .value("CANNOT_DESTROY", fp::CANNOT_DESTROY)
        .value("CANNOT_CONNECT", fp::CANNOT_CONNECT)
        .value("NOT_VALID", fp::NOT_VALID)
        .value("INVALID_SERVICE", fp::INVALID_SERVICE)
        .value("INVALID_DAEMON", fp::INVALID_DAEMON)
        .value("INVALID_MSG", fp::INVALID_MSG)
        .value("INVALID_SUBJECT", fp::INVALID_SUBJECT)
        .value("INVALID_NAME", fp::INVALID_NAME)
        .value("INVALID_QUEUE", fp::INVALID_QUEUE)
        .value("INVALID_TRANSPORT", fp::INVALID_TRANSPORT)
        .value("INVALID_TRANSPORT_STATE", fp::INVALID_TRANSPORT_STATE)
        .value("NOT_FOUND", fp::NOT_FOUND)
        .value("CONVERSION_FAILED", fp::CONVERSION_FAILED)
        .value("UPDATE_FAILED", fp::UPDATE_FAILED)
        .value("TIMEOUT", fp::TIMEOUT)
        .export_values();

    m.def("initialise", &fp::Session::initialise);
    m.def("destroy", &fp::Session::destroy);
    m.def("is_started", &fp::Session::is_started);

    py::class_<PyEvent>(m, "Event")
        .alias<fp::Event>()
        .def(py::init<fp::Queue *>());

    py::class_<fp::TimerEvent>(m, "TimerEvent", py::base<fp::Event>())
        .def("reset", &fp::TimerEvent::reset)
        .def("setTimeout", [] (fp::TimerEvent *event, const uint64_t &timeout) {
                event->setTimeout(std::chrono::microseconds(timeout));
            })
        .def("timeout", [] (fp::TimerEvent *event) noexcept {
                return event->timeout().count();
            });

    py::class_<fp::SignalEvent>(m, "SignalEvent", py::base<fp::Event>())
        .def("signal", &fp::SignalEvent::signal);

    py::class_<fp::IOEvent>(m, "IOEvent", py::base<fp::Event>())
        .def("eventTypes", &fp::IOEvent::eventTypes);

    py::class_<fp::Subscriber>(m, "Subscriber")
        .def(py::init<fp::Transport *, const char *, const std::function<void(const fp::Subscriber *, fp::Message *)> &>())
        .def("transport", &fp::Subscriber::transport)
        .def("subject", &fp::Subscriber::subject);

    py::class_<PyQueue>(m, "Queue")
        .alias<fp::Queue>()
        .def(py::init<>())
        .def("dispatch", (const fp::status(fp::Queue::*)()) &fp::Queue::dispatch)
        .def("dispatch_with_timeout", [](fp::Queue *queue, const uint64_t timeout) {
                return queue->dispatch(std::chrono::milliseconds(timeout));
            })
        .def("try_dispatch", &fp::Queue::try_dispatch)
        .def("eventsInQueue", &fp::Queue::eventsInQueue)
        .def("sendMessage", (fp::status(fp::Transport::*)(const fp::Message &)) &fp::Transport::sendMessage)
        .def("event_count", &fp::Queue::event_count)
        .def("registerDataEvent", (fp::DataEvent *(fp::Queue::*)(const int, const fp::EventType, const std::function<void(fp::DataEvent *, const fp::EventType)> &)) &fp::Queue::registerEvent, "", py::return_value_policy::reference)
        .def("registerTimerEvent", [](fp::Queue *queue, const uint64_t microseconds, const std::function<void(fp::TimerEvent *)> &callback) {
               return  queue->registerEvent(std::chrono::microseconds(microseconds), callback);
            }, "", py::return_value_policy::reference)
        .def("registerSignalEvent", (fp::SignalEvent *(fp::Queue::*)(const int, const std::function<void(fp::SignalEvent *, int)> &)) &fp::Queue::registerEvent, "", py::return_value_policy::reference)
        .def("unregisterDataEvent", (fp::status(fp::Queue::*)(fp::DataEvent *)) &fp::Queue::unregisterEvent)
        .def("unregisterTimerEvent", (fp::status(fp::Queue::*)(fp::TimerEvent *)) &fp::Queue::unregisterEvent)
        .def("unregisterSignalEvent", (fp::status(fp::Queue::*)(fp::SignalEvent *)) &fp::Queue::unregisterEvent)
        .def("addSubscriber", (fp::status(fp::Queue::*)(fp::Subscriber &)) &fp::Queue::addSubscriber)
        .def("removeSubscriber", (fp::status(fp::Queue::*)(fp::Subscriber &)) &fp::Queue::removeSubscriber);


    py::class_<fp::BlockingQueue>(m, "BlockingQueue", py::base<fp::Queue>())
        .def(py::init<>());

    py::class_<fp::BusySpinQueue>(m, "BusySpinQueue", py::base<fp::Queue>())
        .def(py::init<>());

    py::class_<PyTransport>(m, "Transport")
        .alias<fp::Transport>()
        .def(py::init<const char *>())
        .def("valid", &fp::Transport::valid)
        .def("description", &fp::Transport::description)
        .def("sendMessage", (fp::status(fp::Transport::*)(const fp::Message &)) &fp::Transport::sendMessage);

    py::class_<fp::TCPTransport>(m, "TCPTransport", py::base<fp::Transport>())
        .def(py::init<const char *, const char *>());

    py::class_<fp::Message>(m, "Message")
        .def(py::init<>())
        .def("subject", &fp::Message::subject)
        .def("clear", &fp::Message::clear)
        .def("flags", &fp::Message::flags)
        .def("size", &fp::BaseMessage::size)
        .def("getBooleanField", (bool(fp::BaseMessage::*)(const char *, bool &) const) &fp::BaseMessage::getScalarField)
        .def("getUint8Field", (bool(fp::BaseMessage::*)(const char *, uint8_t &) const) &fp::BaseMessage::getScalarField)
        .def("getUint16Field", (bool(fp::BaseMessage::*)(const char *, uint16_t &) const) &fp::BaseMessage::getScalarField)
        .def("getUint32Field", (bool(fp::BaseMessage::*)(const char *, uint32_t &) const) &fp::BaseMessage::getScalarField)
        .def("getUint64Field", (bool(fp::BaseMessage::*)(const char *, uint64_t &) const) &fp::BaseMessage::getScalarField)
        .def("getInt8Field", (bool(fp::BaseMessage::*)(const char *, int8_t &) const) &fp::BaseMessage::getScalarField)
        .def("getInt16Field", (bool(fp::BaseMessage::*)(const char *, int16_t &) const) &fp::BaseMessage::getScalarField)
        .def("getInt32Field", (bool(fp::BaseMessage::*)(const char *, int32_t &) const) &fp::BaseMessage::getScalarField)
        .def("getInt64Field", (bool(fp::BaseMessage::*)(const char *, int64_t &) const) &fp::BaseMessage::getScalarField)
        .def("getFloat32Field", (bool(fp::BaseMessage::*)(const char *, float32_t &) const) &fp::BaseMessage::getScalarField)
        .def("getFloat64Field", (bool(fp::BaseMessage::*)(const char *, float64_t &) const) &fp::BaseMessage::getScalarField)
        //.def("getDataField", (bool(fp::BaseMessage::*)(const char *, const char **, size_t &) const) &fp::BaseMessage::getDataField)
        .def("getStringField", [] (fp::BaseMessage *p, const char *field, std::string &value) {
                   const char *v = nullptr;
                   size_t l = 0;
                   p->getDataField(field, &v, l); 
                })
        .def("__repr__", [](const fp::Message &message) {
                    std::ostringstream os;
                    os << message;
                    return os.str();
                });

    py::class_<fp::MutableMessage>(m, "MutableMessage", py::base<fp::Message>())
        .def(py::init<>())
        .def("setSubject", &fp::MutableMessage::setSubject)
        .def("addBooleanField", (bool(fp::MutableMessage::*)(const char *, const bool&)) &fp::MutableMessage::addScalarField)
        .def("addUint8Field", (bool(fp::MutableMessage::*)(const char *, const uint8_t&)) &fp::MutableMessage::addScalarField)
        .def("addUint16Field", (bool(fp::MutableMessage::*)(const char *, const uint16_t&)) &fp::MutableMessage::addScalarField)
        .def("addUint32Field", (bool(fp::MutableMessage::*)(const char *, const uint32_t&)) &fp::MutableMessage::addScalarField)
        .def("addUint64Field", (bool(fp::MutableMessage::*)(const char *, const uint64_t&)) &fp::MutableMessage::addScalarField)
        .def("addInt8Field", (bool(fp::MutableMessage::*)(const char *, const int8_t&)) &fp::MutableMessage::addScalarField)
        .def("addInt16Field", (bool(fp::MutableMessage::*)(const char *, const int16_t&)) &fp::MutableMessage::addScalarField)
        .def("addInt32Field", (bool(fp::MutableMessage::*)(const char *, const int32_t&)) &fp::MutableMessage::addScalarField)
        .def("addInt64Field", (bool(fp::MutableMessage::*)(const char *, const int64_t&)) &fp::MutableMessage::addScalarField)
        .def("addFloat32Field", (bool(fp::MutableMessage::*)(const char *, const float32_t&)) &fp::MutableMessage::addScalarField)
        .def("addFloat64Field", (bool(fp::MutableMessage::*)(const char *, const float64_t&)) &fp::MutableMessage::addScalarField)
        .def("addStringField", (bool(fp::MutableMessage::*)(const char *, const char *)) &fp::MutableMessage::addDataField)
        .def("addDataTimeField", (bool(fp::MutableMessage::*)(const char *, const uint64_t seconds, const uint64_t microseconds)) &fp::MutableMessage::addDateTimeField)
        .def("addDateTimeFieldMS", [] (fp::MutableMessage *p, const char *field, const uint64_t &value) {
                   return p->addDateTimeField(field, std::chrono::microseconds(value)); 
                })
//        .def("addMessageField", &fp::MutableMessage::addMessageField);
        .def("removeField", &fp::MutableMessage::removeField);

    return m.ptr();
}
