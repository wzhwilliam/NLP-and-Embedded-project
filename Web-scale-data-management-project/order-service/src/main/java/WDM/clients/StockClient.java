package WDM.clients;

import WDM.config.MultipartSupportConfig;
import WDM.pojo.Stock;
import org.springframework.cloud.openfeign.FeignClient;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;

@FeignClient(value = "stock-service", configuration = MultipartSupportConfig.class)
public interface StockClient {
    @GetMapping("stock/find/{itemId}")
    Stock findPrice(@PathVariable("itemId") long itemId);

    @PostMapping("stock/subtract/{itemId}/{amount}")
    ResponseEntity subtract(@PathVariable("itemId") long id, @PathVariable("amount") int amount);

    @PostMapping("stock/add/{itemId}/{amount}")
    ResponseEntity add(@PathVariable("itemId") long id, @PathVariable("amount") int amount);

    @PostMapping("stock/findStock/{itemId}")
    int findStock(@PathVariable("itemId") long itemId);
}
