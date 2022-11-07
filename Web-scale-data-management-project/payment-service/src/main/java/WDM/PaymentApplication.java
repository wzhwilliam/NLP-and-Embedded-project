package WDM;

import com.github.yitter.contract.IdGeneratorOptions;
import com.github.yitter.idgen.YitIdHelper;
import WDM.clients.OrderClient;
import WDM.clients.StockClient;
import io.seata.spring.annotation.datasource.EnableAutoDataSourceProxy;
import org.mybatis.spring.annotation.MapperScan;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.cloud.openfeign.EnableFeignClients;

@MapperScan("WDM.mapper")
@SpringBootApplication
@EnableFeignClients(clients = {OrderClient.class, StockClient.class})
@EnableAutoDataSourceProxy
public class PaymentApplication {
    public static void main(String[] args) {
        // Generate random id using snowflake algorithm
        IdGeneratorOptions options = new IdGeneratorOptions((short) 2);
        YitIdHelper.setIdGenerator(options);

        SpringApplication.run(PaymentApplication.class, args);
    }
}