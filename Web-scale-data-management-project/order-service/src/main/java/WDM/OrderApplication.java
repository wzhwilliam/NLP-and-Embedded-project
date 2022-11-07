package WDM;


import com.github.yitter.contract.IdGeneratorOptions;
import com.github.yitter.idgen.YitIdHelper;
import WDM.clients.PaymentClient;
import WDM.clients.StockClient;
import io.seata.spring.annotation.datasource.EnableAutoDataSourceProxy;
import org.mybatis.spring.annotation.MapperScan;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.cloud.openfeign.EnableFeignClients;


@MapperScan("WDM.mapper")
@SpringBootApplication
@EnableFeignClients(clients = {StockClient.class, PaymentClient.class})
@EnableAutoDataSourceProxy

public class OrderApplication {
    public static void main(String[] args) {
        // Generate random id using snowflake algorithm
        IdGeneratorOptions options = new IdGeneratorOptions((short) 1);
        YitIdHelper.setIdGenerator(options);

        SpringApplication.run(OrderApplication.class, args);
    }
}