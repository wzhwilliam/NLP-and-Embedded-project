package WDM.controller;

import WDM.service.OrderService;
import WDM.utils.ResponseCode;
import WDM.pojo.Order;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

@RestController
@RequestMapping("orders")
public class OrderController {

    @Autowired
    OrderService orderService;

    //    /orders/create/{user_id}
    //    POST - creates an order for the given user, and returns an order_id
    //    Output JSON fields:
    //            “order_id”  - the order’s id
    @PostMapping("create/{userId}")
    public Map<String, String> createOrder(@PathVariable("userId") long userId) {
        Map<String, String> map = new HashMap<>();
        long orderId = orderService.createOrder(userId);
        map.put("order_id", String.valueOf(orderId));
        return map;

    }

    ///orders/remove/{order_id}
    //    DELETE - deletes an order by ID
    @DeleteMapping("remove/{orderId}")
    public ResponseEntity removeOrder(@PathVariable("orderId") long orderId) {
        if (orderService.removeOrder(orderId)) {
            return new ResponseCode().ok();
        } else {
            return new ResponseCode().error();
        }
    }

    ///orders/find/{order_id}
    //    GET - retrieves the information of an order (id, payment status, items included and user id)
    //    Output JSON fields:
    //            “order_id”  - the order’s id
    //“paid” (true/false)
    //            “items”  - list of item ids that are included in the order
    //“user_id”  - the user’s id that made the order
    //“total_cost” - the total cost of the items in the order
    @GetMapping("find/{orderId}")
    public Order findOrder(@PathVariable("orderId") long orderId) {
        return orderService.findOrder(orderId);
    }

    ///orders/addItem/{order_id}/{item_id}
    //    POST - adds a given item in the order given
    @PostMapping("addItem/{orderId}/{itemId}")
    public ResponseEntity addItem(@PathVariable("orderId") long orderId, @PathVariable("itemId") long itemId) {
        if (orderService.addItem(orderId, itemId)) {
            return new ResponseCode().ok();
        } else {
            return new ResponseCode().error();
        }
    }

    ///orders/removeItem/{order_id}/{item_id}
    //    DELETE - removes the given item from the given order
    @DeleteMapping("removeItem/{orderId}/{itemId}")
    public ResponseEntity removeItem(@PathVariable("orderId") long orderId, @PathVariable("itemId") long itemId) {
        if (orderService.removeItem(orderId, itemId)) {
            return new ResponseCode().ok();//return "200";

        } else {
            return new ResponseCode().error();//return "400";
        }
    }

    Lock lock = new ReentrantLock();

    ///orders/checkout/{order_id}
    //    POST - makes the payment (via calling the payment service), subtracts the stock (via the stock service) and returns a status (success/failure).
    @PostMapping("checkout/{orderId}")
    public ResponseEntity checkout(@PathVariable("orderId") long orderId) {
        try {
//            lock.lock();
            Order order = orderService.findOrder(orderId);
            if (orderService.checkout(order)) {
                return new ResponseCode().ok();
            } else {
                return new ResponseCode().error();
            }
        } catch (Exception e) {
            return new ResponseCode().error();
        } finally {
//            lock.unlock();
        }
    }

    @PostMapping("cancel/{orderId}")
    public void cancel(@PathVariable("orderId") long orderId) {
        orderService.cancelOrder(orderId);
    }
}
