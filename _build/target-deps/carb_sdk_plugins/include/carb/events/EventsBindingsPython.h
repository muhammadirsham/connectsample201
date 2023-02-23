// Copyright (c) 2019-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
#pragma once

#include "../BindingsPythonUtils.h"
#include "../Framework.h"
#include "../dictionary/DictionaryBindingsPython.h"
#include "EventsUtils.h"
#include "IEvents.h"

#include <memory>
#include <string>
#include <vector>

DISABLE_PYBIND11_DYNAMIC_CAST(carb::events::IEvents)
DISABLE_PYBIND11_DYNAMIC_CAST(carb::events::ISubscription)
DISABLE_PYBIND11_DYNAMIC_CAST(carb::events::IEvent)
DISABLE_PYBIND11_DYNAMIC_CAST(carb::events::IEventStream)

namespace carb
{
namespace events
{

namespace
{

class PythonEventListener : public IEventListener
{
public:
    PythonEventListener(std::function<void(IEvent*)> fn) : m_fn(std::move(fn))
    {
    }

    void onEvent(IEvent* e) override
    {
        carb::callPythonCodeSafe(m_fn, e);
    }

private:
    std::function<void(IEvent*)> m_fn;

    CARB_IOBJECT_IMPL
};


inline void definePythonModule(py::module& m)
{
    using namespace carb::events;

    m.def("acquire_events_interface", []() { return getCachedEventsInterface(); }, py::return_value_policy::reference,
          py::call_guard<py::gil_scoped_release>());

    m.def("type_from_string", [](const char* s) { return carb::events::typeFromString(s); },
          py::call_guard<py::gil_scoped_release>());

    py::class_<ISubscription, ObjectPtr<ISubscription>>(m, "ISubscription", R"(
        Subscription holder.
        )")
        .def("unsubscribe", &ISubscription::unsubscribe, py::call_guard<py::gil_scoped_release>())

        ;

    py::class_<IEvent, ObjectPtr<IEvent>>(m, "IEvent", R"(
        Event.

        Event has an Event type, a sender id and a payload. Payload is a dictionary like item with arbitrary data.
        )")
        .def_readonly("type", &IEvent::type)
        .def_readonly("sender", &IEvent::sender)
        .def_readonly("payload", &IEvent::payload)

        .def("consume", &IEvent::consume, "Consume event to stop it propagating to other listeners down the line.",
             py::call_guard<py::gil_scoped_release>());


    py::class_<IEventStream, ObjectPtr<IEventStream>>(m, "IEventStream")
        .def("create_subscription_to_pop",
             [](IEventStream* stream, std::function<void(IEvent*)> onEventFn, Order order, const char* name) {
                 return stream->createSubscriptionToPop(
                     carb::stealObject(new PythonEventListener(std::move(onEventFn))).get(), order, name);
             },
             R"(
            Subscribes to event dispatching on the stream.

            See :class:`.Subscription` for more information on subscribing mechanism.

            Args:
                fn: The callback to be called on event dispatch.

            Returns:
                The subscription holder.)",
             py::arg("fn"), py::arg("order") = kDefaultOrder, py::arg("name") = "",
             py::call_guard<py::gil_scoped_release>())

        .def("create_subscription_to_pop_by_type",
             [](IEventStream* stream, EventType eventType, const std::function<void(IEvent*)>& onEventFn, Order order,
                const char* name) {
                 return stream->createSubscriptionToPopByType(
                     eventType, carb::stealObject(new PythonEventListener(onEventFn)).get(), order, name);
             },
             R"(
            Subscribes to event dispatching on the stream.

            See :class:`.Subscription` for more information on subscribing mechanism.

            Args:
                event_type: Event type to listen to.
                fn: The callback to be called on event dispatch.

            Returns:
                The subscription holder.)",
             py::arg("event_type"), py::arg("fn"), py::arg("order") = kDefaultOrder, py::arg("name") = "",
             py::call_guard<py::gil_scoped_release>())

        .def("create_subscription_to_push",
             [](IEventStream* stream, const std::function<void(IEvent*)>& onEventFn, Order order, const char* name) {
                 return stream->createSubscriptionToPush(
                     carb::stealObject(new PythonEventListener(onEventFn)).get(), order, name);
             },
             R"(
            Subscribes to pushing events into stream.

            See :class:`.Subscription` for more information on subscribing mechanism.

            Args:
                fn: The callback to be called on event push.

            Returns:
                The subscription holder.)",
             py::arg("fn"), py::arg("order") = kDefaultOrder, py::arg("name") = "",
             py::call_guard<py::gil_scoped_release>())
        .def("create_subscription_to_push_by_type",
             [](IEventStream* stream, EventType eventType, const std::function<void(IEvent*)>& onEventFn, Order order,
                const char* name) {
                 return stream->createSubscriptionToPushByType(
                     eventType, carb::stealObject(new PythonEventListener(onEventFn)).get(), order, name);
             },
             R"(
            Subscribes to pushing events into stream.

            See :class:`.Subscription` for more information on subscribing mechanism.

            Args:
                event_type: Event type to listen to.
                fn: The callback to be called on event push.

            Returns:
                The subscription holder.)",
             py::arg("event_type"), py::arg("fn"), py::arg("order") = kDefaultOrder, py::arg("name") = "",
             py::call_guard<py::gil_scoped_release>())
        .def_property_readonly("event_count", &IEventStream::getCount, py::call_guard<py::gil_scoped_release>())
        .def("set_subscription_to_pop_order", &IEventStream::setSubscriptionToPopOrder,
             R"(
            Set subscription to pop order by name of subscription.
            )",
             py::arg("name"), py::arg("order"), py::call_guard<py::gil_scoped_release>())
        .def("set_subscription_to_push_order", &IEventStream::setSubscriptionToPushOrder,
             R"(
            Set subscription to push order by name of subscription.
            )",
             py::arg("name"), py::arg("order"), py::call_guard<py::gil_scoped_release>())
        .def("get_subscription_to_pop_order",
             [](IEventStream* self, const char* subscriptionName) -> py::object {
                 Order order;
                 bool b;
                 {
                     py::gil_scoped_release nogil;
                     b = self->getSubscriptionToPopOrder(subscriptionName, &order);
                 }
                 if (b)
                     return py::int_(order);
                 return py::none();
             },
             R"(
            Get subscription to pop order by name of subscription. Return None if subscription was not found.
            )",
             py::arg("name"))
        .def("get_subscription_to_push_order",
             [](IEventStream* self, const char* subscriptionName) -> py::object {
                 Order order;
                 bool b;
                 {
                     py::gil_scoped_release nogil;
                     b = self->getSubscriptionToPushOrder(subscriptionName, &order);
                 }
                 if (b)
                     return py::int_(order);
                 return py::none();
             },
             R"(
            Get subscription to push order by name of subscription. Return None if subscription was not found.
            )",
             py::arg("name"))
        .def("pop", &IEventStream::pop,
             R"(
            Pop event.

            This function blocks execution until there is an event to pop.

            Returns:
                (:class:`.Event`) object. You own this object, it can be stored.
            )",
             py::call_guard<py::gil_scoped_release>())
        .def("try_pop", &IEventStream::tryPop,
             R"(
            Try pop event.

            Returns:
                Pops (:class:`.Event`) if stream is not empty or return `None`.
            )",
             py::call_guard<py::gil_scoped_release>()

                 )
        .def("pump", &IEventStream::pump,
             R"(
            Pump event stream.

            Dispatches all events in a stream.
            )",
             py::call_guard<py::gil_scoped_release>()

                 )

        .def("push",
             [](IEventStream* self, EventType eventType, SenderId sender, py::dict dict) {
                 ObjectPtr<IEvent> e;
                 {
                     py::gil_scoped_release nogil;
                     e = self->createEvent(eventType, sender);
                 }
                 carb::dictionary::setPyObject(carb::dictionary::getDictionary(), e->payload, nullptr, dict);
                 {
                     py::gil_scoped_release nogil;
                     self->push(e.get());
                 }
             },
             R"(
            Push :class:`.Event` into stream.

            Args:
                event_type (int): :class:`.Event` type.
                sender (int): Sender id. Unique can be acquired using :func:`.acquire_unique_sender_id`.
                dict (typing.Dict): :class:`.Event` payload.
            )",
             py::arg("event_type") = 0, py::arg("sender") = 0, py::arg("payload") = py::dict())
        .def("dispatch",
             [](IEventStream* self, EventType eventType, SenderId sender, py::dict dict) {
                 ObjectPtr<IEvent> e;
                 {
                     py::gil_scoped_release nogil;
                     e = self->createEvent(eventType, sender);
                 }
                 carb::dictionary::setPyObject(carb::dictionary::getDictionary(), e->payload, nullptr, dict);
                 {
                     py::gil_scoped_release nogil;
                     self->dispatch(e.get());
                 }
             },
             R"(
            Dispatch :class:`.Event` immediately without putting it into stream.

            Args:
                event_type (int): :class:`.Event` type.
                sender (int): Sender id. Unique can be acquired using :func:`.acquire_unique_sender_id`.
                dict (typing.Dict): :class:`.Event` payload.
            )",
             py::arg("event_type") = 0, py::arg("sender") = 0, py::arg("payload") = py::dict());

    CARB_IGNOREWARNING_MSC_WITH_PUSH(5205)
    py::class_<IEvents>(m, "IEvents")
        .def("create_event_stream", &IEvents::createEventStream,
             R"(
            Create new `.EventStream`.
            )",
             py::call_guard<py::gil_scoped_release>()

                 )
        .def("acquire_unique_sender_id", &IEvents::acquireUniqueSenderId,
             R"(
            Acquire unique sender id.

            Call :func:`.release_unique_sender_id` when it is not needed anymore. It can be reused then.
            )",
             py::call_guard<py::gil_scoped_release>())
        .def("release_unique_sender_id", &IEvents::releaseUniqueSenderId, py::call_guard<py::gil_scoped_release>());
    CARB_IGNOREWARNING_MSC_POP
}
} // namespace

} // namespace events
} // namespace carb
