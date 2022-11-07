package WDM.clients;

import WDM.config.MultipartSupportConfig;
import WDM.pojo.Payment;
import org.springframework.cloud.openfeign.FeignClient;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;

@FeignClient(value = "payment-service", configuration = MultipartSupportConfig.class)
public interface PaymentClient {
    @PostMapping("payment/pay/{userId}/{orderId}/{amount}")
    ResponseEntity pay(@PathVariable("userId") long userid, @PathVariable("orderId") long orderId, @PathVariable("amount") double amount);

    @GetMapping("payment/find_user/{userId}")
    Payment getCredit(@PathVariable("userId") long userId);

}
