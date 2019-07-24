#include "ProducerImpl.h"
#include "rocketmq.h"
#include "rocketmq-client-cpp-full.h"
#include "SendResultONS.h"
#include "Logger.h"
#include "common/UtilAll.h"

using namespace ons;

ProducerImpl::ProducerImpl() throw(ONSClientException) {
}

ProducerImpl::ProducerImpl(ONSFactoryProperty factoryProperty) throw(ons::ONSClientException) {
    std::string logPath(factoryProperty.getLogPath());
    rocketmq::spd_log::initLogger(logPath, rocketmq::LOGLEVEL_INFO);
    graal_isolatethread_t *thread_;
    ThreadAttachment attachment(&thread_);
    factory_property fp;
    FactoryPropertyConverter converter(factoryProperty, fp);
    instanceIndex_ = create_producer(thread_, &fp);
    spdlog::info("Create Producer OK, InstanceId:{}, ProducerID:{}, NameServer:{}",
                 instanceIndex_, factoryProperty.getProducerId(), factoryProperty.getNameSrvAddr());
}

ProducerImpl::~ProducerImpl() {
    rocketmq::spd_log::destoryLogger();
}

void ProducerImpl::start() {

}

void ProducerImpl::shutdown() {
    graal_isolatethread_t *thread_;
    ThreadAttachment attachment(&thread_);
    destroy_instance(thread_, instanceIndex_);
    spdlog::info("Destroy Producer instance {} OK", instanceIndex_);
}

ons::SendResultONS ProducerImpl::send(Message &msg) throw(ONSClientException) {
    message m;
    MessageConverter converter(m, msg);
    send_result sendResult;
    SendResultWrapper wrapper(sendResult);
    graal_isolatethread_t *thread_;
    ThreadAttachment attachment(&thread_);
    send_message(thread_, instanceIndex_, &m, &sendResult);
    if (sendResult.error_no) {
        throw ONSClientException(std::string(sendResult.error_msg), sendResult.error_no);
    }
    spdlog::debug("Send message OK. MsgId: {}", sendResult.message_id);
    SendResultONS sendResultOns;
    sendResultOns.setMessageId(std::string(sendResult.message_id));
    return sendResultOns;
}

SendResultONS ProducerImpl::send(Message &msg, const MessageQueueONS &mq) throw(ons::ONSClientException) {
    return send(msg);
}

#ifdef __cplusplus
extern "C" {
#endif
void on_success_func(void *thread, char *message_id, char *send_callback_ons) {
    auto sendCallbackONS = reinterpret_cast<SendCallbackONS *>(send_callback_ons);
    SendResultONS sendResultOns;
    sendResultOns.setMessageId(message_id);
    sendCallbackONS->onSuccess(sendResultOns);
}

void on_exception_func(void *thread, char *m_msg, int m_error, char *send_callback_ons) {
    auto sendCallbackONS = reinterpret_cast<SendCallbackONS *>(send_callback_ons);
    ONSClientException onsClientException(m_msg, m_error);
    sendCallbackONS->onException(onsClientException);
}
#ifdef __cplusplus
}
#endif

void ProducerImpl::sendAsync(Message &msg, SendCallbackONS *pSendCallback) throw(ons::ONSClientException) {
    message m;
    MessageConverter converter(m, msg);
    send_result sendResult;
    SendResultWrapper wrapper(sendResult);
    callback_func c_f;
    if (pSendCallback == NULL) {
        std::string msg = "Send Callback cannot be NULL.";
        throw ONSClientException(msg, -1);
    }
    c_f.on_success = on_success_func;
    c_f.on_exception = on_exception_func;
    c_f.send_callback_ons = reinterpret_cast<char *>(pSendCallback);

    graal_isolatethread_t *thread_;
    ThreadAttachment attachment(&thread_);
    send_message_async(thread_, instanceIndex_, &m, &sendResult, &c_f);
    if (sendResult.error_no) {
        throw ONSClientException(std::string(sendResult.error_msg), sendResult.error_no);
    }
}

void ProducerImpl::sendOneway(Message &msg) throw(ons::ONSClientException) {
    message m;
    MessageConverter converter(m, msg);
    send_result sendResult;
    bzero(&sendResult, sizeof(send_result));
    SendResultWrapper wrapper(sendResult);
    graal_isolatethread_t *thread_;
    ThreadAttachment attachment(&thread_);
    send_message_oneway(thread_, instanceIndex_, &m, &sendResult);
    if (sendResult.error_no) {
        throw ONSClientException(std::string(sendResult.error_msg), sendResult.error_no);
    }
}