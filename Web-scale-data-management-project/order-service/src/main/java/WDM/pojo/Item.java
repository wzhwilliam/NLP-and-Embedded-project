package WDM.pojo;

import lombok.Data;

import java.util.List;

@Data
public class Item {
    long orderId;
    long itemId;
    double price;
    int amount;
}
